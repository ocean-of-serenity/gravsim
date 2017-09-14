
package main


import (
    "fmt"
    "io"
    "log"
    "math"
    "os"
    "runtime"
    "strings"
    "math/rand"

    "github.com/go-gl/gl/v4.5-core/gl"
    "github.com/go-gl/glfw/v3.2/glfw"
    mgl "github.com/go-gl/mathgl/mgl32"
)


type Color struct {
    r, g, b uint8
}

type Light struct {
    pos, color mgl.Vec4
}

type Directions struct {
    startLeft, startRight, startUp, startDown bool
    stopLeft, stopRight, stopUp, stopDown bool
}

type Camera struct {
    root, watch mgl.Vec3
}


const (
    initWindowWidth = 1280
    initWindowHeight = 720
    minWindowWidth = 512
    minWindowHeight = 288
)

const (
    RotationSpeed = (2 * math.Pi) / 8
)

const (
    numSpheres = 256 * 256
)


var directions Directions

var scrolling bool
var scrollDirection float32

var camera Camera = Camera{mgl.Vec3{3, 4, 10}, mgl.Vec3{0, 0, 0}}

var animating bool


var distPerSecFactor float64 = 16
var distPerSec float64 = (2 * math.Pi) / distPerSecFactor


func main() {
    if err := glfw.Init(); err != nil {
        log.Fatalln("Failed to initialize glfw:", err)
    }
    defer glfw.Terminate()

    glfw.WindowHint(glfw.ContextVersionMajor, 4)
    glfw.WindowHint(glfw.ContextVersionMinor, 5)
    glfw.WindowHint(glfw.OpenGLProfile, glfw.OpenGLCoreProfile)

    window, err := glfw.CreateWindow(
        initWindowWidth,
        initWindowHeight,
        "OpenGL Test",
        nil,
        nil,
    )
    if err != nil {
        log.Fatalln("Failed to create window", err)
    }
    defer window.Destroy()

    window.MakeContextCurrent()
    window.SetSizeLimits(
        minWindowWidth,
        minWindowHeight,
        glfw.DontCare,
        glfw.DontCare,
    )

    if err := gl.Init(); err != nil {
        log.Fatalln("Failed to initialize glow", err)
    }

    log.Println("OpenGL version:", gl.GoStr(gl.GetString(gl.VERSION)))

    glfw.SwapInterval(1)


    var generalProgram uint32
    {
        vs, err := newShader("general_vs.glsl", gl.VERTEX_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        fs, err := newShader("general_fs.glsl", gl.FRAGMENT_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        generalProgram, err = newGeneralProgram(vs, fs)
        if err != nil {
            log.Fatalln(err)
        }
        defer gl.DeleteProgram(generalProgram)

        gl.DeleteShader(fs)
        gl.DeleteShader(vs)
    }

    var tessProgram uint32
    {
        vs, err := newShader("sphere_vs.glsl", gl.VERTEX_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        tcs, err := newShader("sphere_tcs.glsl", gl.TESS_CONTROL_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        tes, err := newShader("sphere_tes.glsl", gl.TESS_EVALUATION_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        fs, err := newShader("sphere_fs.glsl", gl.FRAGMENT_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        tessProgram, err = newTessProgram(vs, tcs, tes, fs)
        if err != nil {
            log.Fatalln(err)
        }
        defer gl.DeleteProgram(tessProgram)

        gl.DeleteShader(fs)
        gl.DeleteShader(tes)
        gl.DeleteShader(tcs)
        gl.DeleteShader(vs)
    }

    var computeProgram uint32
    {
        cs, err := newShader("sphere_cs.glsl", gl.COMPUTE_SHADER)
        if err != nil {
            log.Fatalln(err)
        }

        computeProgram, err = newComputeProgram(cs)
        if err != nil {
            log.Fatalln(err)
        }
        defer gl.DeleteProgram(computeProgram)

        gl.DeleteShader(cs)
    }


    projection := mgl.Perspective(
        math.Pi / 4,
        float32(initWindowWidth) / float32(initWindowHeight),
        0.1,
        40,
    )
    tpProjLoc := gl.GetUniformLocation(tessProgram, gl.Str("projection\x00"))
    gpProjLoc := gl.GetUniformLocation(generalProgram, gl.Str("projection\x00"))
    gl.ProgramUniformMatrix4fv(
        tessProgram,
        tpProjLoc,
        1,
        false,
        &projection[0],
    )
    gl.ProgramUniformMatrix4fv(
        generalProgram,
        gpProjLoc,
        1,
        false,
        &projection[0],
    )

    resizeWindow := func(_ *glfw.Window, width, height int) {
        gl.Viewport(0, 0, int32(width), int32(height))

        projection = mgl.Perspective(
            math.Pi / 4 * (float32(height) / float32(initWindowHeight)),
            float32(width) / float32(height),
            0.1,
            40,
        )

        gl.ProgramUniformMatrix4fv(
            tessProgram,
            tpProjLoc,
            1,
            false,
            &projection[0],
        )
        gl.ProgramUniformMatrix4fv(
            generalProgram,
            gpProjLoc,
            1,
            false,
            &projection[0],
        )
    }
    window.SetFramebufferSizeCallback(resizeWindow)

    view := mgl.LookAtV(
        camera.root,
        camera.watch,
        mgl.Vec3{0, 1, 0},
    )
    tpViewLoc := gl.GetUniformLocation(tessProgram, gl.Str("view\x00"))
    gpViewLoc := gl.GetUniformLocation(generalProgram, gl.Str("view\x00"))
    gl.ProgramUniformMatrix4fv(
        tessProgram,
        tpViewLoc,
        1,
        false,
        &view[0],
    )
    gl.ProgramUniformMatrix4fv(
        generalProgram,
        gpViewLoc,
        1,
        false,
        &view[0],
    )

    window.SetKeyCallback(
        func(
            _ *glfw.Window,
            key glfw.Key,
            _ int,
            action glfw.Action,
            _ glfw.ModifierKey,
        ) {
            switch key {
            case glfw.KeyA:
                switch action {
                case glfw.Press:
                    directions.startLeft = true
                case glfw.Release:
                    directions.stopLeft = true
                }
            case glfw.KeyD:
                switch action {
                case glfw.Press:
                    directions.startRight = true
                case glfw.Release:
                    directions.stopRight = true
                }
            case glfw.KeyW:
                switch action {
                case glfw.Press:
                    directions.startUp = true
                case glfw.Release:
                    directions.stopUp = true
                }
            case glfw.KeyS:
                switch action {
                case glfw.Press:
                    directions.startDown = true
                case glfw.Release:
                    directions.stopDown = true
                }
            case glfw.KeyR:
                switch action {
                case glfw.Press:
                    animating = !animating
                }
            case glfw.KeyQ:
                switch action {
                case glfw.Press:
                    if distPerSecFactor > 0.015625 {
                        distPerSecFactor /= 2
                        distPerSec = (2 * math.Pi) / distPerSecFactor
                    }
                }
            case glfw.KeyE:
                switch action {
                case glfw.Press:
                    if distPerSecFactor < 16384 {
                        distPerSecFactor *= 2
                        distPerSec = (2 * math.Pi) / distPerSecFactor
                    }
                }
            case glfw.KeyF:
                switch action {
                case glfw.Press:
                    distPerSecFactor = 16
                    distPerSec = (2 * math.Pi) / distPerSecFactor
                }
            }
        },
    )

    window.SetScrollCallback(func(_ *glfw.Window, xOffset, yOffset float64) {
        scrollDirection = float32(yOffset)
        scrolling = true
    })


    var lightUbo uint32
    {
        gl.CreateBuffers(1, &lightUbo)
        gl.NamedBufferStorage(lightUbo, 2 * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(lightUbo, gl.WRITE_ONLY)
            light := (*[1]Light)(ptr)[:]
            light[0] = Light{mgl.Vec4{0, 0, 0, 1}, mgl.Vec4{1, 1, 1, 1}}
            gl.UnmapNamedBuffer(lightUbo)
        }
    }
    gl.UniformBlockBinding(tessProgram, 1, 1)
    gl.BindBufferBase(gl.UNIFORM_BUFFER, 1, lightUbo)

    tpAmbLoc := gl.GetUniformLocation(tessProgram, gl.Str("ambient_part\x00"))
    gl.ProgramUniform1f(tessProgram, tpAmbLoc, 0.4)

    tpCamLoc := gl.GetUniformLocation(
        tessProgram,
        gl.Str("camera_location\x00"),
    )
    gl.ProgramUniform3fv(
        tessProgram,
        tpCamLoc,
        1,
        &camera.root[0],
    )


    cpDistLoc := gl.GetUniformLocation(computeProgram, gl.Str("distance\x00"))


    var socVao uint32
    gl.CreateVertexArrays(1, &socVao)
    {
        var vbo uint32
        gl.CreateBuffers(1, &vbo)
        gl.NamedBufferStorage(vbo, 6 * 4 * 3, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vbo, gl.WRITE_ONLY)
            positions := (*[6]mgl.Vec3)(ptr)[:]
            positions[0] = mgl.Vec3{1, 0, 0}
            positions[1] = mgl.Vec3{-1, 0, 0}
            positions[2] = mgl.Vec3{0, 1, 0}
            positions[3] = mgl.Vec3{0, -1, 0}
            positions[4] = mgl.Vec3{0, 0, 1}
            positions[5] = mgl.Vec3{0, 0, -1}
            gl.UnmapNamedBuffer(vbo)
        }
        gl.VertexArrayVertexBuffer(socVao, 0, vbo, 0, 3 * 4)
        gl.EnableVertexArrayAttrib(socVao, 0)
        gl.VertexArrayAttribBinding(socVao, 0, 0)
        gl.VertexArrayAttribFormat(socVao, 0, 3, gl.FLOAT, false, 0)

        var vco uint32
        gl.CreateBuffers(1, &vco)
        gl.NamedBufferStorage(vco, 6 * 3, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vco, gl.WRITE_ONLY)
            colors := (*[6]Color)(ptr)[:]
            colors[0] = Color{255, 0, 0}
            colors[1] = Color{0, 0, 255}
            colors[2] = Color{255, 0, 0}
            colors[3] = Color{0, 0, 255}
            colors[4] = Color{255, 0, 0}
            colors[5] = Color{0, 0, 255}
            gl.UnmapNamedBuffer(vco)
        }
        gl.VertexArrayVertexBuffer(socVao, 1, vco, 0, 3)
        gl.EnableVertexArrayAttrib(socVao, 1)
        gl.VertexArrayAttribBinding(socVao, 1, 1)
        gl.VertexArrayAttribFormat(socVao, 1, 3, gl.UNSIGNED_BYTE, true, 0)

        var imbo uint32
        gl.CreateBuffers(1, &imbo)
        gl.NamedBufferStorage(imbo, 4 * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(imbo, gl.WRITE_ONLY)
            models := (*[1]mgl.Mat4)(ptr)[:]
            models[0] = mgl.Scale3D(12, 12, 12)
            gl.UnmapNamedBuffer(imbo)
        }
        gl.VertexArrayVertexBuffer(socVao, 2, imbo, 0, 4 * 4 * 4)
        gl.VertexArrayBindingDivisor(socVao, 2, 1)
        for i := uint32(0); i < 4; i++ {
            gl.EnableVertexArrayAttrib(socVao, 2 + i)
            gl.VertexArrayAttribBinding(socVao, 2 + i, 2)
            gl.VertexArrayAttribFormat(
                socVao,
                2 + i,
                4,
                gl.FLOAT,
                false,
                4 * 4 * i,
            )
        }
    }

    var sphereVao, sphereImbo, sphereIabo uint32
    gl.CreateVertexArrays(1, &sphereVao)
    {
        var vbo uint32
        gl.CreateBuffers(1, &vbo)
        gl.NamedBufferStorage(vbo, 6 * 4 * 3, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vbo, gl.WRITE_ONLY)
            positions := (*[6]mgl.Vec3)(ptr)[:]
            positions[0] = mgl.Vec3{0, 1, 0}
            positions[1] = mgl.Vec3{1, 0, 0}
            positions[2] = mgl.Vec3{0, 0, -1}
            positions[3] = mgl.Vec3{-1, 0, 0}
            positions[4] = mgl.Vec3{0, 0, 1}
            positions[5] = mgl.Vec3{0, -1, 0}
            gl.UnmapNamedBuffer(vbo)
        }
        gl.VertexArrayVertexBuffer(sphereVao, 0, vbo, 0, 3 * 4)
        gl.EnableVertexArrayAttrib(sphereVao, 0)
        gl.VertexArrayAttribBinding(sphereVao, 0, 0)
        gl.VertexArrayAttribFormat(sphereVao, 0, 3, gl.FLOAT, false, 0)

        var ebo uint32
        gl.CreateBuffers(1, &ebo)
        gl.NamedBufferStorage(ebo, 24 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(ebo, gl.WRITE_ONLY)
            elements := (*[24]uint32)(ptr)[:]
            elements[0] = 0
            elements[1] = 1
            elements[2] = 2
            elements[3] = 0
            elements[4] = 2
            elements[5] = 3
            elements[6] = 0
            elements[7] = 3
            elements[8] = 4
            elements[9] = 0
            elements[10] = 4
            elements[11] = 1
            elements[12] = 5
            elements[13] = 1
            elements[14] = 2
            elements[15] = 5
            elements[16] = 2
            elements[17] = 3
            elements[18] = 5
            elements[19] = 3
            elements[20] = 4
            elements[21] = 5
            elements[22] = 4
            elements[23] = 1
            gl.UnmapNamedBuffer(ebo)
        }
        gl.VertexArrayElementBuffer(sphereVao, ebo)

        var vco uint32
        gl.CreateBuffers(1, &vco)
        gl.NamedBufferStorage(vco, numSpheres * 3, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vco, gl.WRITE_ONLY)
            colors := (*[numSpheres]Color)(ptr)[:]
            colors[0] = Color{255, 255, 255}
            for i := 1; i < numSpheres; i++ {
                colors[i] = Color{
                    uint8(rand.Float32() * 255),
                    uint8(rand.Float32() * 255),
                    uint8(rand.Float32() * 255),
                }
            }
            gl.UnmapNamedBuffer(vco)
        }
        gl.VertexArrayVertexBuffer(sphereVao, 1, vco, 0, 3)
        gl.VertexArrayBindingDivisor(sphereVao, 1, 1)
        gl.EnableVertexArrayAttrib(sphereVao, 1)
        gl.VertexArrayAttribBinding(sphereVao, 1, 1)
        gl.VertexArrayAttribFormat(sphereVao, 1, 3, gl.UNSIGNED_BYTE, true, 0)

        var positions [numSpheres]mgl.Vec3
        positions[0] = mgl.Vec3{0, 0, 0}
        for i := 1; i < numSpheres; i++ {
            positions[i] = mgl.Vec3{
                rand.Float32() * 24 - 12,
                rand.Float32() * 24 - 12,
                rand.Float32() * 24 - 12,
            }
        }

        gl.CreateBuffers(1, &sphereImbo)
        gl.NamedBufferStorage(sphereImbo, numSpheres * 4 * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(sphereImbo, gl.WRITE_ONLY)
            models := (*[numSpheres]mgl.Mat4)(ptr)[:]

            models[0] = mgl.Translate3D(0, 0, 0).Mul4(mgl.Scale3D(0.4, 0.4, 0.4))
            for i := 1; i < numSpheres; i++ {
                x := positions[i].X()
                y := positions[i].Y()
                z := positions[i].Z()
                models[i] = mgl.Translate3D(x, y, z).Mul4(mgl.Scale3D(0.04, 0.04, 0.04))
            }

            gl.UnmapNamedBuffer(sphereImbo)
        }

        gl.VertexArrayVertexBuffer(sphereVao, 2, sphereImbo, 0, 4 * 4 * 4)
        gl.VertexArrayBindingDivisor(sphereVao, 2, 1)
        for i := uint32(0); i < 4; i++ {
            gl.EnableVertexArrayAttrib(sphereVao, 2 + i)
            gl.VertexArrayAttribBinding(sphereVao, 2 + i, 2)
            gl.VertexArrayAttribFormat(
                sphereVao,
                2 + i,
                4,
                gl.FLOAT,
                false,
                4 * 4 * i,
            )
        }

        gl.CreateBuffers(1, &sphereIabo)
        gl.NamedBufferStorage(sphereIabo, numSpheres * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            perpDir := func(direction mgl.Vec3) mgl.Vec3 {
                switch {
                case direction.X() != 0:
                    return mgl.Vec3{
                        (-direction.Y() - direction.Z()) / direction.X(),
                        1,
                        1,
                    }
                case direction.Y() != 0:
                    return mgl.Vec3{
                        1,
                        (-direction.X() - direction.Z()) / direction.Y(),
                        1,
                    }
                case direction.Z() != 0:
                    return mgl.Vec3{
                        1,
                        1,
                        (-direction.X() - direction.Y()) / direction.Z(),
                    }
                default:
                    return direction
                }
            }

            ptr := gl.MapNamedBuffer(sphereIabo, gl.WRITE_ONLY)
            axiis := (*[numSpheres]mgl.Vec4)(ptr)[:]

            axiis[0] = mgl.Vec4{0, 0, 0, 1}
            for i := 1; i < numSpheres; i++ {
                axiis[i] = perpDir(positions[i]).Vec4(1)
            }

            gl.UnmapNamedBuffer(sphereIabo)
        }
    }

    gl.ShaderStorageBlockBinding(computeProgram, 1, 1)
    gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 1, sphereImbo)

    gl.ShaderStorageBlockBinding(computeProgram, 2, 2)
    gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 2, sphereIabo)


    gl.Enable(gl.DEPTH_TEST)

    gl.ClearColor(0, 0, 0, 1)


    var time, loopStart, secondsPerFrame float64
    for !window.ShouldClose() {
        loopStart = glfw.GetTime()

        // animating
        if animating {
            gl.ProgramUniform1f(computeProgram, cpDistLoc, float32(secondsPerFrame * distPerSec))
            gl.UseProgram(computeProgram)
            gl.DispatchCompute(256, 1, 1)
            gl.UseProgram(0)
        }

        // input handling
        if directions.startLeft {
            if !directions.startRight {
                rotate := mgl.HomogRotate3D(
                    -float32(secondsPerFrame * RotationSpeed),
                    mgl.Vec3{0, 1, 0},
                )
                camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
                view = mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
                gl.ProgramUniformMatrix4fv(
                    tessProgram,
                    tpViewLoc,
                    1,
                    false,
                    &view[0],
                )
                gl.ProgramUniformMatrix4fv(
                    generalProgram,
                    gpViewLoc,
                    1,
                    false,
                    &view[0],
                )

                gl.ProgramUniform3fv(
                    tessProgram,
                    tpCamLoc,
                    1,
                    &camera.root[0],
                )
            }

            if directions.stopLeft {
                directions.startLeft = false
                directions.stopLeft = false
            }
        }

        if directions.startRight {
            if !directions.startLeft {
                rotate := mgl.HomogRotate3D(
                    float32(secondsPerFrame * RotationSpeed),
                    mgl.Vec3{0, 1, 0},
                )
                camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
                view = mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
                gl.ProgramUniformMatrix4fv(
                    tessProgram,
                    tpViewLoc,
                    1,
                    false,
                    &view[0],
                )
                gl.ProgramUniformMatrix4fv(
                    generalProgram,
                    gpViewLoc,
                    1,
                    false,
                    &view[0],
                )

                gl.ProgramUniform3fv(
                    tessProgram,
                    tpCamLoc,
                    1,
                    &camera.root[0],
                )
            }

            if directions.stopRight {
                directions.startRight = false
                directions.stopRight = false
            }
        }

        if directions.startUp {
            upDot := mgl.Vec3{0, 1, 0}.Dot(camera.root.Normalize())
            if !directions.startDown && upDot < 0.99 {
                axis := mgl.Vec3{0, 1, 0}.Cross(camera.root).Normalize()
                rotate := mgl.HomogRotate3D(
                    -float32(secondsPerFrame * RotationSpeed),
                    axis,
                )
                camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
                view = mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
                gl.ProgramUniformMatrix4fv(
                    tessProgram,
                    tpViewLoc,
                    1,
                    false,
                    &view[0],
                )
                gl.ProgramUniformMatrix4fv(
                    generalProgram,
                    gpViewLoc,
                    1,
                    false,
                    &view[0],
                )

                gl.ProgramUniform3fv(
                    tessProgram,
                    tpCamLoc,
                    1,
                    &camera.root[0],
                )
            }

            if directions.stopUp {
                directions.startUp = false
                directions.stopUp = false
            }
        }

        if directions.startDown {
            downDot := mgl.Vec3{0, -1, 0}.Dot(camera.root.Normalize())
            if !directions.startUp && downDot < 0.99 {
                axis := mgl.Vec3{0, 1, 0}.Cross(camera.root).Normalize()
                rotate := mgl.HomogRotate3D(
                    float32(secondsPerFrame * RotationSpeed),
                    axis,
                )
                camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
                view = mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
                gl.ProgramUniformMatrix4fv(
                    tessProgram,
                    tpViewLoc,
                    1,
                    false,
                    &view[0],
                )
                gl.ProgramUniformMatrix4fv(
                    generalProgram,
                    gpViewLoc,
                    1,
                    false,
                    &view[0],
                )

                gl.ProgramUniform3fv(
                    tessProgram,
                    tpCamLoc,
                    1,
                    &camera.root[0],
                )
            }

            if directions.stopDown {
                directions.startDown = false
                directions.stopDown = false
            }
        }

        if scrolling {
            scroll := camera.root.Normalize().Mul(-scrollDirection / 3)
            newRoot := camera.root.Add(scroll)
            newRootLength := newRoot.Len()
            if newRootLength > 1 && newRootLength < 30 {
                camera.root = newRoot
                view = mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
                gl.ProgramUniformMatrix4fv(
                    tessProgram,
                    tpViewLoc,
                    1,
                    false,
                    &view[0],
                )
                gl.ProgramUniformMatrix4fv(
                    generalProgram,
                    gpViewLoc,
                    1,
                    false,
                    &view[0],
                )

                gl.ProgramUniform3fv(
                    tessProgram,
                    tpCamLoc,
                    1,
                    &camera.root[0],
                )
            }

            scrolling = false
        }

        // rendering
        gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

        gl.UseProgram(generalProgram)
        gl.BindVertexArray(socVao)
        gl.DrawArraysInstanced(gl.LINES, 0, 6, 1)
        gl.BindVertexArray(0)
        gl.UseProgram(0)

        gl.UseProgram(tessProgram)
        gl.BindVertexArray(sphereVao)
        gl.DrawElementsInstanced(gl.PATCHES, 24, gl.UNSIGNED_INT, nil, numSpheres)
        gl.BindVertexArray(0)
        gl.UseProgram(0)

        window.SwapBuffers()

        // event handling
        glfw.PollEvents()

        secondsPerFrame = glfw.GetTime() - loopStart
        time += secondsPerFrame
    }
}


func init() {
    runtime.LockOSThread()
}


func newShader(fileName string, shaderType uint32) (uint32, error) {
    file, err := os.Open(fileName)
    if err != nil {
        return 0, fmt.Errorf("Could not open '%s': %s", fileName, err)
    }
    defer file.Close()

    fi, err := file.Stat()
    if err != nil {
        return 0, fmt.Errorf("Could not obtain info on '%s': %s", fileName, err)
    }

    bSource := make([]byte, fi.Size())
    _, err = file.Read(bSource)
    if err != io.EOF && err != nil {
        return 0, fmt.Errorf("Could not read from '%s': %s", fileName, err)
    }

    source := string(bSource) + "\x00"

    shader := gl.CreateShader(shaderType)
    if shader == 0 {
        return 0, fmt.Errorf("Could not create name for shader '%s'!", fileName)
    }

    cSources, free := gl.Strs(source)
    gl.ShaderSource(shader, 1, cSources, nil)
    free()

    gl.CompileShader(shader)

    var compileStatus int32
    gl.GetShaderiv(shader, gl.COMPILE_STATUS, &compileStatus)
    if compileStatus == gl.FALSE {
        var infoLogLength int32
        gl.GetShaderiv(shader, gl.INFO_LOG_LENGTH, &infoLogLength)

        infoLog := strings.Repeat("\x00", int(infoLogLength + 1))
        gl.GetShaderInfoLog(shader, infoLogLength, nil, gl.Str(infoLog))

        gl.DeleteShader(shader)

        return 0, fmt.Errorf(
            "Failed to compile '%s'!\n#>> Source:\n%s\n#>> InfoLog:\n%s",
            fileName,
            source,
            infoLog,
        )
    }

    return shader, nil
}


func newGeneralProgram(vs, fs uint32) (uint32, error) {
    program := gl.CreateProgram()
    if program == 0 {
        return 0, fmt.Errorf("Could not create name for program!")
    }

    gl.AttachShader(program, vs)
    gl.AttachShader(program, fs)
    gl.LinkProgram(program)
    gl.DetachShader(program, vs)
    gl.DetachShader(program, fs)

    var validateStatus int32
    gl.GetProgramiv(program, gl.VALIDATE_STATUS, &validateStatus)

    var linkStatus int32
    gl.GetProgramiv(program, gl.LINK_STATUS, &linkStatus)

    if validateStatus == gl.FALSE || linkStatus == gl.FALSE {
        var infoLogLength int32
        gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &infoLogLength)

        infoLog := strings.Repeat("\x00", int(infoLogLength + 1))
        gl.GetProgramInfoLog(program, infoLogLength, nil, gl.Str(infoLog))

        gl.DeleteProgram(program)

        return 0, fmt.Errorf(
            "Failed to link program!\n#>> InfoLog:\n%s",
            infoLog,
        )
    }

    return program, nil
}


func newTessProgram(vs, tcs, tes, fs uint32) (uint32, error) {
    program := gl.CreateProgram()
    if program == 0 {
        return 0, fmt.Errorf("Could not create name for program!")
    }

    gl.AttachShader(program, vs)
    gl.AttachShader(program, tcs)
    gl.AttachShader(program, tes)
    gl.AttachShader(program, fs)
    gl.LinkProgram(program)
    gl.DetachShader(program, vs)
    gl.DetachShader(program, tcs)
    gl.DetachShader(program, tes)
    gl.DetachShader(program, fs)

    var validateStatus int32
    gl.GetProgramiv(program, gl.VALIDATE_STATUS, &validateStatus)

    var linkStatus int32
    gl.GetProgramiv(program, gl.LINK_STATUS, &linkStatus)

    if validateStatus == gl.FALSE || linkStatus == gl.FALSE {
        var infoLogLength int32
        gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &infoLogLength)

        infoLog := strings.Repeat("\x00", int(infoLogLength + 1))
        gl.GetProgramInfoLog(program, infoLogLength, nil, gl.Str(infoLog))

        gl.DeleteProgram(program)

        return 0, fmt.Errorf(
            "Failed to link program!\n#>> InfoLog:\n%s",
            infoLog,
        )
    }

    return program, nil
}


func newComputeProgram(cs uint32) (uint32, error) {
    program := gl.CreateProgram()
    if program == 0 {
        return 0, fmt.Errorf("Could not create name for program!")
    }

    gl.AttachShader(program, cs)
    gl.LinkProgram(program)
    gl.DetachShader(program, cs)

    var validateStatus int32
    gl.GetProgramiv(program, gl.VALIDATE_STATUS, &validateStatus)

    var linkStatus int32
    gl.GetProgramiv(program, gl.LINK_STATUS, &linkStatus)

    if validateStatus == gl.FALSE || linkStatus == gl.FALSE {
        var infoLogLength int32
        gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &infoLogLength)

        infoLog := strings.Repeat("\x00", int(infoLogLength + 1))
        gl.GetProgramInfoLog(program, infoLogLength, nil, gl.Str(infoLog))

        gl.DeleteProgram(program)

        return 0, fmt.Errorf(
            "Failed to link program!\n#>> InfoLog:\n%s",
            infoLog,
        )
    }

    return program, nil
}


