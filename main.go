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
    "github.com/go-gl/mathgl/mgl32"
)

type Color struct {
    r, g, b, a uint8
}

func main() {
    if err := glfw.Init(); err != nil {
        log.Fatalln("Failed to initialize glfw:", err)
    }
    defer glfw.Terminate()

    glfw.WindowHint(glfw.Resizable, glfw.False)
    glfw.WindowHint(glfw.ContextVersionMajor, 4)
    glfw.WindowHint(glfw.ContextVersionMinor, 5)
    glfw.WindowHint(glfw.OpenGLProfile, glfw.OpenGLCoreProfile)
    glfw.WindowHint(glfw.Samples, 16)

    windowWidth := 1280
    windowHeight := 720

    window, err := glfw.CreateWindow(
        windowWidth,
        windowHeight,
        "OpenGL Test",
        nil,
        nil,
    )
    if err != nil {
        log.Fatalln("Failed to create window", err)
    }
    defer window.Destroy()

    window.MakeContextCurrent()

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

    projection := mgl32.Perspective(
        math.Pi/4,
        float32(windowWidth)/float32(windowHeight),
        4,
        20,
    )

    view := mgl32.LookAtV(
        mgl32.Vec3{3, 4, 10},
        mgl32.Vec3{0, 0, 0},
        mgl32.Vec3{0, 1, 0},
    )

    viewProjection := projection.Mul4(view)

    var socVao uint32
    gl.CreateVertexArrays(1, &socVao)
    {
        var vbo uint32
        gl.CreateBuffers(1, &vbo)
        gl.NamedBufferStorage(vbo, 6 * 4 * 3, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vbo, gl.WRITE_ONLY)
            positions := (*[6]mgl32.Vec3)(ptr)[:]
            positions[0] = mgl32.Vec3{1, 0, 0}
            positions[1] = mgl32.Vec3{-1, 0, 0}
            positions[2] = mgl32.Vec3{0, 1, 0}
            positions[3] = mgl32.Vec3{0, -1, 0}
            positions[4] = mgl32.Vec3{0, 0, 1}
            positions[5] = mgl32.Vec3{0, 0, -1}
            gl.UnmapNamedBuffer(vbo)
        }
        gl.VertexArrayVertexBuffer(socVao, 0, vbo, 0, 3 * 4)
        gl.EnableVertexArrayAttrib(socVao, 0)
        gl.VertexArrayAttribBinding(socVao, 0, 0)
        gl.VertexArrayAttribFormat(socVao, 0, 3, gl.FLOAT, false, 0)

        var vco uint32
        gl.CreateBuffers(1, &vco)
        gl.NamedBufferStorage(vco, 6 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vco, gl.WRITE_ONLY)
            colors := (*[6]Color)(ptr)[:]
            colors[0] = Color{255, 0, 0, 255}
            colors[1] = Color{0, 0, 255, 255}
            colors[2] = Color{255, 0, 0, 255}
            colors[3] = Color{0, 0, 255, 255}
            colors[4] = Color{255, 0, 0, 255}
            colors[5] = Color{0, 0, 255, 255}
            gl.UnmapNamedBuffer(vco)
        }
        gl.VertexArrayVertexBuffer(socVao, 1, vco, 0, 4)
        gl.EnableVertexArrayAttrib(socVao, 1)
        gl.VertexArrayAttribBinding(socVao, 1, 1)
        gl.VertexArrayAttribFormat(socVao, 1, 4, gl.UNSIGNED_BYTE, true, 0)

        var imbo uint32
        gl.CreateBuffers(1, &imbo)
        gl.NamedBufferStorage(imbo, 4 * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(imbo, gl.WRITE_ONLY)
            models := (*[1]mgl32.Mat4)(ptr)[:]
            models[0] = viewProjection.Mul4(mgl32.Scale3D(6, 6, 6))
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
            positions := (*[6]mgl32.Vec3)(ptr)[:]
            positions[0] = mgl32.Vec3{0, 1, 0}
            positions[1] = mgl32.Vec3{1, 0, 0}
            positions[2] = mgl32.Vec3{0, 0, -1}
            positions[3] = mgl32.Vec3{-1, 0, 0}
            positions[4] = mgl32.Vec3{0, 0, 1}
            positions[5] = mgl32.Vec3{0, -1, 0}
            gl.UnmapNamedBuffer(vbo)
        }
        gl.VertexArrayVertexBuffer(sphereVao, 0, vbo, 0, 3 * 4)
        gl.EnableVertexArrayAttrib(sphereVao, 0)
        gl.VertexArrayAttribBinding(sphereVao, 0, 0)
        gl.VertexArrayAttribFormat(sphereVao, 0, 3, gl.FLOAT, false, 0)

        var vco uint32
        gl.CreateBuffers(1, &vco)
        gl.NamedBufferStorage(vco, 6 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(vco, gl.WRITE_ONLY)
            colors := (*[6]Color)(ptr)[:]
            colors[0] = Color{255,   0,   0, 255}
            colors[1] = Color{  0, 255,   0, 255}
            colors[2] = Color{255, 255, 255, 255}
            colors[3] = Color{  0, 255,   0, 255}
            colors[4] = Color{255, 255, 255, 255}
            colors[5] = Color{  0,   0, 255, 255}
            gl.UnmapNamedBuffer(vco)
        }
        gl.VertexArrayVertexBuffer(sphereVao, 1, vco, 0, 4)
        gl.EnableVertexArrayAttrib(sphereVao, 1)
        gl.VertexArrayAttribBinding(sphereVao, 1, 1)
        gl.VertexArrayAttribFormat(sphereVao, 1, 4, gl.UNSIGNED_BYTE, true, 0)

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

        gl.CreateBuffers(1, &imbo)
        gl.NamedBufferStorage(imbo, 1 * 4 * 4 * 4, nil, gl.MAP_WRITE_BIT)
        {
            ptr := gl.MapNamedBuffer(imbo, gl.WRITE_ONLY)
            models := (*[1]mgl32.Mat4)(ptr)[:]
            models[0] = viewProjection.Mul4(mgl32.Translate3D(2, 2, 0))
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


    gl.PolygonMode(gl.FRONT_AND_BACK, gl.LINE)

    gl.Enable(gl.DEPTH_TEST)

    gl.Enable(gl.MULTISAMPLE)

    gl.ClearColor(0, 0, 0, 1)


    perpDir := func(direction mgl32.Vec3) mgl32.Vec3 {
        switch {
        case direction.X() != 0:
            return mgl32.Vec3{
                (-direction.Y() - direction.Z()) / direction.X(),
                1,
                1,
            }
        case direction.Y() != 0:
            return mgl32.Vec3{
                1,
                (-direction.X() - direction.Z()) / direction.Y(),
                1,
            }
        case direction.Z() != 0:
            return mgl32.Vec3{
                1,
                1,
                (-direction.X() - direction.Y()) / direction.Z(),
            }
        default:
            return direction
        }
    }

    pDir := perpDir(mgl32.Vec3{2, 2, 0}.Normalize()).Normalize()
    distPerSec := (2 * math.Pi) / 16
    var time, loopStart float64
    for !window.ShouldClose() {
        loopStart = glfw.GetTime()

        // animating
        {
            rotate := mgl32.HomogRotate3D(float32(time * distPerSec), pDir)
            translate := mgl32.Translate3D(2, 2, 0)

            ptr := gl.MapNamedBuffer(imbo, gl.WRITE_ONLY)
            models := (*[1]mgl32.Mat4)(ptr)[:]
            models[0] = viewProjection.Mul4(rotate).Mul4(translate)
            gl.UnmapNamedBuffer(imbo)
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

        time += glfw.GetTime() - loopStart
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


