
package main


import (
    "fmt"
    "io"
    "log"
    "math"
    "os"
    "runtime"
    "strings"

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

type Material struct {
    ambientStrength, diffuseStrength, specularStrength float32
}

type Camera struct {
    root, watch mgl.Vec4
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


type Directions struct {
    startLeft, startRight, startUp, startDown bool
    stopLeft, stopRight, stopUp, stopDown bool
}

var directions Directions

var cameraRootLocation mgl.Vec3 = mgl.Vec3{3, 4, 10}
var cameraWatchLocation mgl.Vec3 = mgl.Vec3{0, 0, 0}


func main() {
    if err := glfw.Init(); err != nil {
        log.Fatalln("Failed to initialize glfw:", err)
    }
    defer glfw.Terminate()

    glfw.WindowHint(glfw.ContextVersionMajor, 4)
    glfw.WindowHint(glfw.ContextVersionMinor, 5)
    glfw.WindowHint(glfw.OpenGLProfile, glfw.OpenGLCoreProfile)
    glfw.WindowHint(glfw.Samples, 16)

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


    projection := mgl.Perspective(
        math.Pi / 4,
        float32(initWindowWidth) / float32(initWindowHeight),
        4,
        20,
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
            4,
            20,
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
        cameraRootLocation,
        cameraWatchLocation,
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
            scancode int,
            action glfw.Action,
            mods glfw.ModifierKey,
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
            }
        },
    )


    var lightUbo uint32
    gl.CreateBuffers(1, &lightUbo)
    gl.NamedBufferStorage(lightUbo, 8 * 4, nil, gl.MAP_WRITE_BIT)
    {
        ptr := gl.MapNamedBuffer(lightUbo, gl.WRITE_ONLY)
        light := (*[1]Light)(ptr)[:]
        light[0] = Light{mgl.Vec4{0, 0, 0, 1}, mgl.Vec4{1, 1, 1, 1}}
        gl.UnmapNamedBuffer(lightUbo)
    }
    gl.UniformBlockBinding(tessProgram, 1, 1)
    gl.BindBufferRange(gl.UNIFORM_BUFFER, 1, lightUbo, 0, 8 * 4)

    var materialUbo uint32
    gl.CreateBuffers(1, &materialUbo)
    gl.NamedBufferStorage(materialUbo, 3 * 4, nil, gl.MAP_WRITE_BIT)
    {
        ptr := gl.MapNamedBuffer(materialUbo, gl.WRITE_ONLY)
        material := (*[1]Material)(ptr)[:]
        material[0] = Material{0.4, 0.6, 0.8}
        gl.UnmapNamedBuffer(materialUbo)
    }
    gl.UniformBlockBinding(tessProgram, 2, 2)
    gl.BindBufferRange(gl.UNIFORM_BUFFER, 2, materialUbo, 0, 3 * 4)

    var cameraUbo uint32
    gl.CreateBuffers(1, &cameraUbo)
    gl.NamedBufferStorage(cameraUbo, 8 * 4, nil, gl.MAP_WRITE_BIT)
    {
        ptr := gl.MapNamedBuffer(cameraUbo, gl.WRITE_ONLY)
        camera := (*[1]Camera)(ptr)[:]
        camera[0] = Camera{mgl.Vec4{3, 4, 10, 1}, mgl.Vec4{0, 0, 0, 1}}
        gl.UnmapNamedBuffer(cameraUbo)
    }
    gl.UniformBlockBinding(tessProgram, 3, 3)
    gl.BindBufferRange(gl.UNIFORM_BUFFER, 3, cameraUbo, 0, 8 * 4)


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
            models[0] = mgl.Scale3D(6, 6, 6)
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

    var sphereVao uint32
    gl.CreateVertexArrays(1, &sphereVao)
    var imbo uint32
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
        gl.NamedBufferStorage(vco, 1 * 3, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vco, gl.WRITE_ONLY)
            colors := (*[1]Color)(ptr)[:]
            colors[0] = Color{255, 255, 255}
            gl.UnmapNamedBuffer(vco)
        }
        gl.VertexArrayVertexBuffer(sphereVao, 1, vco, 0, 3)
        gl.VertexArrayBindingDivisor(sphereVao, 1, 1)
        gl.EnableVertexArrayAttrib(sphereVao, 1)
        gl.VertexArrayAttribBinding(sphereVao, 1, 1)
        gl.VertexArrayAttribFormat(sphereVao, 1, 3, gl.UNSIGNED_BYTE, true, 0)

        gl.CreateBuffers(1, &imbo)
        gl.NamedBufferStorage(imbo, 1 * 4 * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(imbo, gl.WRITE_ONLY)
            models := (*[1]mgl.Mat4)(ptr)[:]
            models[0] = mgl.Translate3D(2, 2, 0)
            gl.UnmapNamedBuffer(imbo)
        }
        gl.VertexArrayVertexBuffer(sphereVao, 2, imbo, 0, 4 * 4 * 4)
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
    }


//    gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)

    gl.Enable(gl.DEPTH_TEST)

    gl.Enable(gl.MULTISAMPLE)

    gl.ClearColor(0, 0, 0, 1)


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

    pDir := perpDir(mgl.Vec3{2, 2, 0}.Normalize()).Normalize()
    distPerSec := (2 * math.Pi) / 16
    var time, loopStart, secondsPerFrame float64
    for !window.ShouldClose() {
        loopStart = glfw.GetTime()

        // animating
        {
            rotate := mgl.HomogRotate3D(float32(time * distPerSec), pDir)
            translate := mgl.Translate3D(2, 2, 0)

            ptr := gl.MapNamedBuffer(imbo, gl.WRITE_ONLY)
            models := (*[1]mgl.Mat4)(ptr)[:]
            models[0] = rotate.Mul4(translate)
            gl.UnmapNamedBuffer(imbo)
        }

        // input handling
        if directions.startLeft {
            if !directions.startRight {
                rotate := mgl.HomogRotate3D(
                    -float32(secondsPerFrame * RotationSpeed),
                    mgl.Vec3{0, 1, 0},
                )
                cameraRootLocation = rotate.Mul4x1(
                    cameraRootLocation.Vec4(1),
                ).Vec3()
                view = mgl.LookAtV(
                    cameraRootLocation,
                    cameraWatchLocation,
                    mgl.Vec3{0, 1, 0},
                )
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
                ptr := gl.MapNamedBuffer(cameraUbo, gl.WRITE_ONLY)
                camera := (*[1]Camera)(ptr)[:]
                camera[0] = Camera{
                    cameraRootLocation.Vec4(1),
                    cameraWatchLocation.Vec4(1),
                }
                gl.UnmapNamedBuffer(cameraUbo)
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
                cameraRootLocation = rotate.Mul4x1(
                    cameraRootLocation.Vec4(1),
                ).Vec3()
                view = mgl.LookAtV(
                    cameraRootLocation,
                    cameraWatchLocation,
                    mgl.Vec3{0, 1, 0},
                )
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
                ptr := gl.MapNamedBuffer(cameraUbo, gl.WRITE_ONLY)
                camera := (*[1]Camera)(ptr)[:]
                camera[0] = Camera{
                    cameraRootLocation.Vec4(1),
                    cameraWatchLocation.Vec4(1),
                }
                gl.UnmapNamedBuffer(cameraUbo)
            }

            if directions.stopRight {
                directions.startRight = false
                directions.stopRight = false
            }
        }

        if directions.startUp {
            upDot := mgl.Vec3{0, 1, 0}.Dot(cameraRootLocation.Normalize())
            if !directions.startDown && upDot < 0.99 {
                axis := mgl.Vec3{0, 1, 0}.Cross(cameraRootLocation).Normalize()
                rotate := mgl.HomogRotate3D(
                    -float32(secondsPerFrame * RotationSpeed),
                    axis,
                )
                cameraRootLocation = rotate.Mul4x1(
                    cameraRootLocation.Vec4(1),
                ).Vec3()
                view = mgl.LookAtV(
                    cameraRootLocation,
                    cameraWatchLocation,
                    mgl.Vec3{0, 1, 0},
                )
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
                ptr := gl.MapNamedBuffer(cameraUbo, gl.WRITE_ONLY)
                camera := (*[1]Camera)(ptr)[:]
                camera[0] = Camera{
                    cameraRootLocation.Vec4(1),
                    cameraWatchLocation.Vec4(1),
                }
                gl.UnmapNamedBuffer(cameraUbo)
            }

            if directions.stopUp {
                directions.startUp = false
                directions.stopUp = false
            }
        }

        if directions.startDown {
            downDot := mgl.Vec3{0, -1, 0}.Dot(cameraRootLocation.Normalize())
            if !directions.startUp && downDot < 0.99 {
                axis := mgl.Vec3{0, 1, 0}.Cross(cameraRootLocation).Normalize()
                rotate := mgl.HomogRotate3D(
                    float32(secondsPerFrame * RotationSpeed),
                    axis,
                )
                cameraRootLocation = rotate.Mul4x1(
                    cameraRootLocation.Vec4(1),
                ).Vec3()
                view = mgl.LookAtV(
                    cameraRootLocation,
                    cameraWatchLocation,
                    mgl.Vec3{0, 1, 0},
                )
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
                ptr := gl.MapNamedBuffer(cameraUbo, gl.WRITE_ONLY)
                camera := (*[1]Camera)(ptr)[:]
                camera[0] = Camera{
                    cameraRootLocation.Vec4(1),
                    cameraWatchLocation.Vec4(1),
                }
                gl.UnmapNamedBuffer(cameraUbo)
            }

            if directions.stopDown {
                directions.startDown = false
                directions.stopDown = false
            }
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
        gl.DrawElementsInstanced(gl.PATCHES, 24, gl.UNSIGNED_INT, nil, 1)
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


