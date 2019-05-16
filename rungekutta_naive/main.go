
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
	"time"
	"unsafe"
	"reflect"

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

type Camera struct {
	root, watch mgl.Vec3
}

type Location struct {
	location mgl.Vec3
	padding float32
}

type Velocity struct {
	velocity mgl.Vec3
	padding float32
}

type Duration struct {
	forceCompute, sphereRender uint64
}


const (
	initialWindowWidth = 1280
	initialWindowHeight = 720

	cameraRotationDistancePerFrame = ((2 * math.Pi) / 8) * (1 / 64.0)

//	G = 1.887130407e-7		// Lunar Masses, Solar Radii and hours
	G = 1.142602313e-4		// Lunar Masses, Solar Radii and days

	profilingLogLength = 1000
)


var gravityProgram, axisProgram, sphereProgram, axisVertexArray, sphereVertexArray, sphereInstanceColorBuffer uint32
var sphereInstanceModelBuffer, gravityLocationBuffer0, gravityLocationBuffer1, gravityMassBuffer, gravityVelocityBuffer, query uint32

var axisProgramView, sphereProgramView, sphereProgramCameraLocation, gravityProgramNumSpheres int32

var camera Camera = Camera{mgl.Vec3{8000.0, 12000.0, 16000.0}, mgl.Vec3{0, 0, 0}}

var leftKeyPressed, rightKeyPressed, upKeyPressed, downKeyPressed bool
var leftKeyOn, rightKeyOn, upKeyOn, downKeyOn bool
var moveLeft, moveRight, moveUp, moveDown bool

var scrolling bool
var scrollDirection float32

var profilingLog Duration
var profilingFileName string


func main() {
	// misc setup
	profilingFileName = fmt.Sprintf("profile-%s.csv", time.Now().Format("20060102150405"))


	// initialize GLFW and OpenGL
	if err := glfw.Init(); err != nil {
		log.Fatalln("Failed to initialize glfw:", err)
	}
	defer glfw.Terminate()

	glfw.WindowHint(glfw.ContextVersionMajor, 4)
	glfw.WindowHint(glfw.ContextVersionMinor, 5)
	glfw.WindowHint(glfw.OpenGLProfile, glfw.OpenGLCoreProfile)
//	glfw.WindowHint(glfw.OpenGLDebugContext, glfw.True)

	window, err := glfw.CreateWindow(initialWindowWidth, initialWindowHeight, "OpenGL Test", nil, nil)
	if err != nil {
		log.Fatalln("Failed to create window", err)
	}
	defer window.Destroy()

	window.MakeContextCurrent()

	glfw.SwapInterval(0)

	if err := gl.Init(); err != nil {
		log.Fatalln("Failed to initialize glow", err)
	}

//	gl.Enable(gl.DEBUG_OUTPUT_SYNCHRONOUS)
//	gl.DebugMessageCallback(
//		func(source, gltype, id, severity uint32, _ int32, message string, _ unsafe.Pointer) {
//			log.Println(
//				debugSeverityString(severity),
//				debugSourceString(source),
//				debugTypeString(gltype),
//				id,
//				message,
//			)
//		},
//		nil,
//	)

	// enable depth testing in the fragment shader
	gl.Enable(gl.DEPTH_TEST)

	// clear framebuffer
	gl.ClearColor(0, 0, 0, 1)

	log.Println("OpenGL version:", gl.GoStr(gl.GetString(gl.VERSION)))


	// set up OpenGL shaders and programs
	{
		vertexShader, err := newShader("axis_vertex_shader.glsl", gl.VERTEX_SHADER)
		if err != nil {
			log.Fatalln(err)
		}
		fragmentShader, err := newShader("axis_fragment_shader.glsl", gl.FRAGMENT_SHADER)
		if err != nil {
			log.Fatalln(err)
		}
		axisProgram, err = newAxisProgram(vertexShader, fragmentShader)
		if err != nil {
			log.Fatalln(err)
		}
		gl.DeleteShader(fragmentShader)
		gl.DeleteShader(vertexShader)


		vertexShader, err = newShader("sphere_vertex_shader.glsl", gl.VERTEX_SHADER)
		if err != nil {
			log.Fatalln(err)
		}
		tesselationControlShader, err := newShader("sphere_tesselation_control_shader.glsl", gl.TESS_CONTROL_SHADER)
		if err != nil {
			log.Fatalln(err)
		}
		tesselationEvaluationShader, err := newShader("sphere_tesselation_evaluation_shader.glsl", gl.TESS_EVALUATION_SHADER)
		if err != nil {
			log.Fatalln(err)
		}
		fragmentShader, err = newShader("sphere_fragment_shader.glsl", gl.FRAGMENT_SHADER)
		if err != nil {
			log.Fatalln(err)
		}
		sphereProgram, err = newSphereProgram(
			vertexShader,
			tesselationControlShader,
			tesselationEvaluationShader,
			fragmentShader,
		)
		if err != nil {
			log.Fatalln(err)
		}
		gl.DeleteShader(fragmentShader)
		gl.DeleteShader(tesselationEvaluationShader)
		gl.DeleteShader(tesselationControlShader)
		gl.DeleteShader(vertexShader)
	}


	// initialize queries and shader/program uniforms and set up callbacks that might change them during runtime
	{
		gl.GenQueries(1, &query)

		projection := mgl.Perspective(
			math.Pi / 4,
			float32(initialWindowWidth) / float32(initialWindowHeight),
			32.0,
			88000.0,
		)
		sphereProgramProjection := gl.GetUniformLocation(sphereProgram, gl.Str("projection\x00"))
		axisProgramProjection := gl.GetUniformLocation(axisProgram, gl.Str("projection\x00"))
		gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramProjection, 1, false, &projection[0])
		gl.ProgramUniformMatrix4fv(axisProgram, axisProgramProjection, 1, false, &projection[0])

		window.SetFramebufferSizeCallback(
			func(_ *glfw.Window, width, height int) {
				gl.Viewport(0, 0, int32(width), int32(height))

				projection := mgl.Perspective(
					math.Pi / 4 * (float32(height) / float32(initialWindowHeight)),
					float32(width) / float32(height),
					32.0,
					88000.0,
				)

				gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramProjection, 1, false, &projection[0])
				gl.ProgramUniformMatrix4fv(axisProgram, axisProgramProjection, 1, false, &projection[0])
			},
		)

		view := mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
		sphereProgramView = gl.GetUniformLocation(sphereProgram, gl.Str("view\x00"))
		axisProgramView = gl.GetUniformLocation(axisProgram, gl.Str("view\x00"))
		gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramView, 1, false, &view[0])
		gl.ProgramUniformMatrix4fv(axisProgram, axisProgramView, 1, false, &view[0])

		var lightUniformBuffer uint32
		{
			gl.CreateBuffers(1, &lightUniformBuffer)
			{
				var light [1]Light
				light[0] = Light{mgl.Vec4{0, 0, 0, 1}, mgl.Vec4{1, 1, 1, 1}}
				gl.NamedBufferStorage(lightUniformBuffer, 2 * 4 * 4, unsafe.Pointer(&light[0]), 0)
			}
		}
		gl.BindBufferBase(gl.UNIFORM_BUFFER, 0, lightUniformBuffer)

		sphereProgramCameraLocation = gl.GetUniformLocation(sphereProgram, gl.Str("camera_location\x00"))
		gl.ProgramUniform3fv(sphereProgram, sphereProgramCameraLocation, 1, &camera.root[0])

		gravityProgramNumSpheres = gl.GetUniformLocation(gravityProgram, gl.Str("num_spheres\x00"))
	}


	// set up callbacks for camera movement
	window.SetKeyCallback(
		func(_ *glfw.Window, key glfw.Key, _ int, action glfw.Action, _ glfw.ModifierKey) {
			switch key {
			case glfw.KeyA:
				switch action {
				case glfw.Press:
					leftKeyPressed = true
					leftKeyOn = true
				case glfw.Release:
					leftKeyOn = false
				}
			case glfw.KeyD:
				switch action {
				case glfw.Press:
					rightKeyPressed = true
					rightKeyOn = true
				case glfw.Release:
					rightKeyOn = false
				}
			case glfw.KeyW:
				switch action {
				case glfw.Press:
					upKeyPressed = true
					upKeyOn = true
				case glfw.Release:
					upKeyOn = false
				}
			case glfw.KeyS:
				switch action {
				case glfw.Press:
					downKeyPressed = true
					downKeyOn = true
				case glfw.Release:
					downKeyOn = false
				}
			}
		},
	)

	window.SetScrollCallback(
		func(_ *glfw.Window, xOffset, yOffset float64) {
			scrollDirection -= float32(yOffset)
			scrolling = true
		},
	)


	// set up coordinate axis render prerequisites
	gl.CreateVertexArrays(1, &axisVertexArray)
	{
		gl.EnableVertexArrayAttrib(axisVertexArray, 0)
		gl.VertexArrayAttribFormat(axisVertexArray, 0, 3, gl.FLOAT, false, 0)
		gl.VertexArrayAttribBinding(axisVertexArray, 0, 0)
		var vertexBuffer uint32
		gl.CreateBuffers(1, &vertexBuffer)
		{
			var positions [6]mgl.Vec3
			positions[0] = mgl.Vec3{1, 0, 0}
			positions[1] = mgl.Vec3{-1, 0, 0}
			positions[2] = mgl.Vec3{0, 1, 0}
			positions[3] = mgl.Vec3{0, -1, 0}
			positions[4] = mgl.Vec3{0, 0, 1}
			positions[5] = mgl.Vec3{0, 0, -1}
			gl.NamedBufferStorage(vertexBuffer, 6 * 4 * 3, unsafe.Pointer(&positions[0]), 0)
		}
		gl.VertexArrayVertexBuffer(axisVertexArray, 0, vertexBuffer, 0, 3 * 4)

		gl.EnableVertexArrayAttrib(axisVertexArray, 1)
		gl.VertexArrayAttribFormat(axisVertexArray, 1, 3, gl.UNSIGNED_BYTE, true, 0)
		gl.VertexArrayAttribBinding(axisVertexArray, 1, 1)
		var vertexColorBuffer uint32
		gl.CreateBuffers(1, &vertexColorBuffer)
		{
			var colors [6]Color
			colors[0] = Color{255, 0, 0}
			colors[1] = Color{0, 0, 255}
			colors[2] = Color{255, 0, 0}
			colors[3] = Color{0, 0, 255}
			colors[4] = Color{255, 0, 0}
			colors[5] = Color{0, 0, 255}
			gl.NamedBufferStorage(vertexColorBuffer, 6 * 3, unsafe.Pointer(&colors[0]), 0)
		}
		gl.VertexArrayVertexBuffer(axisVertexArray, 1, vertexColorBuffer, 0, 3)

		gl.VertexArrayBindingDivisor(axisVertexArray, 2, 1)
		for i := uint32(0); i < 4; i++ {
			gl.EnableVertexArrayAttrib(axisVertexArray, 2 + i)
			gl.VertexArrayAttribFormat(axisVertexArray, 2 + i, 4, gl.FLOAT, false, 4 * 4 * i)
			gl.VertexArrayAttribBinding(axisVertexArray, 2 + i, 2)
		}
		var instanceModelBuffer uint32
		gl.CreateBuffers(1, &instanceModelBuffer)
		{
			var models [1]mgl.Mat4
			models[0] = mgl.Scale3D(22500.0, 22500.0, 22500.0)
			gl.NamedBufferStorage(instanceModelBuffer, 4 * 4 * 4, unsafe.Pointer(&models[0]), 0)
		}
		gl.VertexArrayVertexBuffer(axisVertexArray, 2, instanceModelBuffer, 0, 4 * 4 * 4)
	}


	// set up sphere render prerequisites
	gl.CreateVertexArrays(1, &sphereVertexArray)
	{
		gl.EnableVertexArrayAttrib(sphereVertexArray, 0)
		gl.VertexArrayAttribFormat(sphereVertexArray, 0, 3, gl.FLOAT, false, 0)
		gl.VertexArrayAttribBinding(sphereVertexArray, 0, 0)
		var vertexBuffer uint32
		gl.CreateBuffers(1, &vertexBuffer)
		{
			var positions [6]mgl.Vec3
			positions[0] = mgl.Vec3{0, 1, 0}
			positions[1] = mgl.Vec3{1, 0, 0}
			positions[2] = mgl.Vec3{0, 0, -1}
			positions[3] = mgl.Vec3{-1, 0, 0}
			positions[4] = mgl.Vec3{0, 0, 1}
			positions[5] = mgl.Vec3{0, -1, 0}
			gl.NamedBufferStorage(vertexBuffer, 6 * 4 * 3, unsafe.Pointer(&positions[0]), 0)
		}
		gl.VertexArrayVertexBuffer(sphereVertexArray, 0, vertexBuffer, 0, 3 * 4)

		var elementBuffer uint32
		gl.CreateBuffers(1, &elementBuffer)
		{
			var elements [24]uint32
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
			gl.NamedBufferStorage(elementBuffer, 24 * 4, unsafe.Pointer(&elements[0]), 0)
		}
		gl.VertexArrayElementBuffer(sphereVertexArray, elementBuffer)

		gl.EnableVertexArrayAttrib(sphereVertexArray, 1)
		gl.VertexArrayAttribFormat(sphereVertexArray, 1, 3, gl.UNSIGNED_BYTE, true, 0)
		gl.VertexArrayBindingDivisor(sphereVertexArray, 1, 1)
		gl.VertexArrayAttribBinding(sphereVertexArray, 1, 1)
		// the size of the buffer for this attribute array changes and will be re-bound in the loop below

		gl.VertexArrayBindingDivisor(sphereVertexArray, 2, 1)
		for i := uint32(0); i < 4; i++ {
			gl.EnableVertexArrayAttrib(sphereVertexArray, 2 + i)
			gl.VertexArrayAttribFormat(sphereVertexArray, 2 + i, 4, gl.FLOAT, false, 4 * 4 * i)
			gl.VertexArrayAttribBinding(sphereVertexArray, 2 + i, 2)
		}
		// ditto here
	}


	// profiling loops
	var localWorkGroupSize uint32
	for localWorkGroupSize = 32; localWorkGroupSize <= 1024; localWorkGroupSize *= 2 {
		var globalWorkGroupSize uint32
		for numSpheres := 2; numSpheres <= 262144; numSpheres *= 2 {
			globalWorkGroupSize = uint32(numSpheres) / localWorkGroupSize
			if uint32(numSpheres) % localWorkGroupSize != 0 {
				globalWorkGroupSize += 1
			}

			profilingLog = Duration{0, 0}


			fmt.Printf("Profiling; Global Workgroup Size: %v, Local Workgroup Size: %v, Spheres: %v\n", globalWorkGroupSize, localWorkGroupSize, numSpheres)


			{
				gl.DeleteProgram(gravityProgram)
				computeShader, err := newGravityShader(localWorkGroupSize, uint32(numSpheres))
				if err != nil {
					log.Fatalln(err)
				}
				gravityProgram, err = newGravityProgram(computeShader)
				if err != nil {
					log.Fatalln(err)
				}
				gl.DeleteShader(computeShader)
			}


			// attach color and model data to sphere vertex array
			{
				gl.DeleteBuffers(1, &sphereInstanceColorBuffer)
				gl.CreateBuffers(1, &sphereInstanceColorBuffer)
				{
					var colors []Color = make([]Color, numSpheres)
					colors[0] = Color{255, 255, 255}
					for i := 1; i < numSpheres; i++ {
						colors[i] = Color{
							uint8(rand.Float32() * 255),
							uint8(rand.Float32() * 255),
							uint8(rand.Float32() * 255),
						}
					}
					shdr := (*reflect.SliceHeader)(unsafe.Pointer(&colors))
					gl.NamedBufferStorage(sphereInstanceColorBuffer, numSpheres * 3, unsafe.Pointer(shdr.Data), 0)
				}
				gl.VertexArrayVertexBuffer(sphereVertexArray, 1, sphereInstanceColorBuffer, 0, 3)

				gl.DeleteBuffers(1, &sphereInstanceModelBuffer)
				gl.CreateBuffers(1, &sphereInstanceModelBuffer)
				{
					var models []mgl.Mat4 = make([]mgl.Mat4, numSpheres)
					models[0] = mgl.Scale3D(864.0, 864.0, 864.0)
					for i := 1; i < numSpheres; i++ {
						models[i] = mgl.Scale3D(28.0, 28.0, 28.0)
					}
					shdr := (*reflect.SliceHeader)(unsafe.Pointer(&models))
					gl.NamedBufferStorage(sphereInstanceModelBuffer, numSpheres * 4 * 4 * 4, unsafe.Pointer(shdr.Data), 0)
				}
				gl.VertexArrayVertexBuffer(sphereVertexArray, 2, sphereInstanceModelBuffer, 0, 4 * 4 * 4)
			}


			// generate orb positions and masses, then calculate corresponding initial velocity
			var orbLocations []Location = make([]Location, numSpheres)
			var orbMasses []float32 = make([]float32, numSpheres)
			var orbMassLocations []mgl.Vec3 = make([]mgl.Vec3, numSpheres)
			var sumOrbMass float32
			var sumOrbMassLocations mgl.Vec3
			orbLocations[0].location = mgl.Vec3{0, 0, 0}
			orbMasses[0] = 1e11
			orbMassLocations[0] = orbLocations[0].location.Mul(orbMasses[0])
			sumOrbMass = orbMasses[0]
			sumOrbMassLocations = orbMassLocations[0]
			for i := 1; i < numSpheres; i++ {
				orbLocations[i].location = mgl.Vec3{
					rand.Float32() - 0.5,
					(rand.Float32() - 0.5) * 0.05,
					rand.Float32() - 0.5,
				}.Normalize().Mul(1000.0 + rand.Float32() * 21000.0)

				orbMasses[i] = float32(math.Pow10(rand.Intn(3))) * rand.Float32()

				orbMassLocations[i] = orbLocations[i].location.Mul(orbMasses[i])

				sumOrbMass += orbMasses[i]

				sumOrbMassLocations = sumOrbMassLocations.Add(orbMassLocations[i])
			}

			var orbVelocities []Velocity = make([]Velocity, numSpheres)
			orbVelocities[0].velocity = mgl.Vec3{0, 0, 0}
			for i := 1; i < numSpheres; i++ {
				// displacement vector from barycenter (without current orb) to current orb
				dv := orbLocations[i].location.Sub(sumOrbMassLocations.Sub(orbMassLocations[i]).Mul(1 / (sumOrbMass - orbMasses[i])))

				// velocity magnitude
				mag := ((sumOrbMass - orbMasses[i]) / sumOrbMass) * float32(math.Sqrt(float64((G * sumOrbMass) / dv.Len())))

				// velocity direction
				dir := dv.Cross(mgl.Vec3{0, 1, 0}).Normalize()

				// initial velocity
				orbVelocities[i].velocity = dir.Mul(mag)
			}


			// copy orb locations into shader storage buffers for use by the shaders
			gl.DeleteBuffers(1, &gravityLocationBuffer0)
			gl.CreateBuffers(1, &gravityLocationBuffer0)
			{
				// populate new buffer with data from data pointer of slice variable 'orbLocations'
				shdr := (*reflect.SliceHeader)(unsafe.Pointer(&orbLocations))
				gl.NamedBufferStorage(gravityLocationBuffer0, numSpheres * 4 * 4, unsafe.Pointer(shdr.Data), 0)
			}
			gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 0, gravityLocationBuffer0)

			// no need to populate this buffer since the first compute dispatch stores the newly calculated positions here anyway
			gl.DeleteBuffers(1, &gravityLocationBuffer1)
			gl.CreateBuffers(1, &gravityLocationBuffer1)
			gl.NamedBufferStorage(gravityLocationBuffer1, numSpheres * 4 * 4, nil, 0)
			gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 1, gravityLocationBuffer1)

			// copy orb masses into shader storage buffers for use by the compute shader
			gl.DeleteBuffers(1, &gravityMassBuffer)
			gl.CreateBuffers(1, &gravityMassBuffer)
			{
				// populate new buffer with data from data pointer of slice variable 'orbLocations'
				shdr := (*reflect.SliceHeader)(unsafe.Pointer(&orbMasses))
				gl.NamedBufferStorage(gravityMassBuffer, numSpheres * 4, unsafe.Pointer(shdr.Data), 0)
			}
			gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 2, gravityMassBuffer)

			// copy orb velocities into shader storage buffers for use by the compute shader
			gl.DeleteBuffers(1, &gravityVelocityBuffer)
			gl.CreateBuffers(1, &gravityVelocityBuffer)
			{
				// populate new buffer with data from data pointer of slice variable 'orbVelocities'
				shdr := (*reflect.SliceHeader)(unsafe.Pointer(&orbVelocities))
				gl.NamedBufferStorage(gravityVelocityBuffer, numSpheres * 4 * 4, unsafe.Pointer(shdr.Data), 0)
			}
			gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 3, gravityVelocityBuffer)


			// main loop; breaks when profiling is done
			var frameCounter uint32
			var timeSinceLastSecond float64
			var loopTimeStart float64 = glfw.GetTime()
			var loopTimeElapsed float64
			var queryReady uint32
			var queryDuration uint64
			var locationBuffer1Active bool
			var progressBar string = ""
			for i := 0; i < profilingLogLength; i++ {
				// time measurements
				loopTimeElapsed = glfw.GetTime() - loopTimeStart
				timeSinceLastSecond += loopTimeElapsed
				loopTimeStart = glfw.GetTime()


				// GLFW event handling
				if window.ShouldClose() {
					return
				}
				glfw.PollEvents()


				// FPS counter and progress bar, displays FPS every second
				if i % 25 == 0 {
					progressBar += "="
				}

				frameCounter += 1
				if timeSinceLastSecond > 1 || i == profilingLogLength - 1 {
				    timeSinceLastSecond = 0
					fmt.Printf("[%-40s] %4d/%4d Frames; %4d FPS\r", progressBar, i + 1, profilingLogLength, frameCounter)
				    frameCounter = 0
				}


				// use compute shader to update sphere positions
				gl.UseProgram(gravityProgram)
				gl.BeginQuery(gl.TIME_ELAPSED, query)
				gl.DispatchCompute(globalWorkGroupSize, 1, 1)
				gl.EndQuery(gl.TIME_ELAPSED)
				gl.UseProgram(0)
				for {
					gl.GetQueryObjectuiv(query, gl.QUERY_RESULT_AVAILABLE, &queryReady)
					if queryReady == gl.TRUE {
						break
					}
				}
				gl.GetQueryObjectui64v(query, gl.QUERY_RESULT, &queryDuration)
				profilingLog.forceCompute += queryDuration


				// input handling with GLFW events, determine movement direction, then move camera accordingly
				if (leftKeyPressed || leftKeyOn) && !(rightKeyPressed || rightKeyOn) {
					moveLeft = true
				} else if (rightKeyPressed || rightKeyOn) && !(leftKeyPressed || leftKeyOn) {
					moveRight = true
				}

				leftKeyPressed, rightKeyPressed = false, false

				if (upKeyPressed || upKeyOn) && !(downKeyPressed || downKeyOn) {
					dot := mgl.Vec3{0, 1, 0}.Dot(camera.root.Normalize())
					if dot < 0.99 {
						moveUp = true
					}
				} else if (downKeyPressed || downKeyOn) && !(upKeyPressed || upKeyOn) {
					dot := mgl.Vec3{0, -1, 0}.Dot(camera.root.Normalize())
					if dot < 0.99 {
						moveDown = true
					}
				}

				upKeyPressed, downKeyPressed = false, false

				if moveLeft {
					rotate := mgl.HomogRotate3D(-cameraRotationDistancePerFrame, mgl.Vec3{0, 1, 0})
					camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
					view := mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
					gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramView, 1, false, &view[0])
					gl.ProgramUniformMatrix4fv(axisProgram, axisProgramView, 1, false, &view[0])
					gl.ProgramUniform3fv(sphereProgram, sphereProgramCameraLocation, 1, &camera.root[0])
				} else if moveRight {
					rotate := mgl.HomogRotate3D(cameraRotationDistancePerFrame, mgl.Vec3{0, 1, 0})
					camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
					view := mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
					gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramView, 1, false, &view[0])
					gl.ProgramUniformMatrix4fv(axisProgram, axisProgramView, 1, false, &view[0])
					gl.ProgramUniform3fv(sphereProgram, sphereProgramCameraLocation, 1, &camera.root[0])
				}

				moveLeft, moveRight = false, false

				if moveUp {
					axis := mgl.Vec3{0, 1, 0}.Cross(camera.root).Normalize()
					rotate := mgl.HomogRotate3D(cameraRotationDistancePerFrame, axis)
					camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
					view := mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
					gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramView, 1, false, &view[0])
					gl.ProgramUniformMatrix4fv(axisProgram, axisProgramView, 1, false, &view[0])
					gl.ProgramUniform3fv(sphereProgram, sphereProgramCameraLocation, 1, &camera.root[0])
				} else if moveDown {
					axis := mgl.Vec3{0, 1, 0}.Cross(camera.root).Normalize()
					rotate := mgl.HomogRotate3D(cameraRotationDistancePerFrame, axis)
					camera.root = rotate.Mul4x1(camera.root.Vec4(1)).Vec3()
					view := mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
					gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramView, 1, false, &view[0])
					gl.ProgramUniformMatrix4fv(axisProgram, axisProgramView, 1, false, &view[0])
					gl.ProgramUniform3fv(sphereProgram, sphereProgramCameraLocation, 1, &camera.root[0])
				}

				moveUp, moveDown = false, false

				if scrolling {
					scroll := camera.root.Normalize().Mul(scrollDirection * 2096.0)
					newRoot := camera.root.Add(scroll)
					newRootLength := newRoot.Len()
					if newRootLength > 1024.0 && newRootLength < 44000.0 {
						camera.root = newRoot
						view := mgl.LookAtV(camera.root, camera.watch, mgl.Vec3{0, 1, 0})
						gl.ProgramUniformMatrix4fv(sphereProgram, sphereProgramView, 1, false, &view[0])
						gl.ProgramUniformMatrix4fv(axisProgram, axisProgramView, 1, false, &view[0])
						gl.ProgramUniform3fv(sphereProgram, sphereProgramCameraLocation, 1, &camera.root[0])
					}

				}

				scrollDirection = 0
				scrolling = false


				// rendering
				gl.Clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT)

				gl.UseProgram(axisProgram)
				gl.BindVertexArray(axisVertexArray)
				gl.DrawArraysInstanced(gl.LINES, 0, 6, 1)
				gl.BindVertexArray(0)
				gl.UseProgram(0)

				gl.UseProgram(sphereProgram)
				gl.BindVertexArray(sphereVertexArray)
				gl.BeginQuery(gl.TIME_ELAPSED, query)
				gl.DrawElementsInstanced(gl.PATCHES, 24, gl.UNSIGNED_INT, nil, int32(numSpheres))
				gl.EndQuery(gl.TIME_ELAPSED)
				gl.BindVertexArray(0)
				gl.UseProgram(0)
				for {
					gl.GetQueryObjectuiv(query, gl.QUERY_RESULT_AVAILABLE, &queryReady)
					if queryReady == gl.TRUE {
						break
					}
				}
				gl.GetQueryObjectui64v(query, gl.QUERY_RESULT, &queryDuration)
				profilingLog.sphereRender += queryDuration

				if locationBuffer1Active {
					gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 0, gravityLocationBuffer0)
					gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 1, gravityLocationBuffer1)
				} else {
					gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 0, gravityLocationBuffer1)
					gl.BindBufferBase(gl.SHADER_STORAGE_BUFFER, 1, gravityLocationBuffer0)
				}
				locationBuffer1Active = !locationBuffer1Active

				window.SwapBuffers()
			}
			fmt.Println()



			// write profiling measurements to filesystem
			file, err := os.OpenFile(profilingFileName, os.O_WRONLY | os.O_CREATE | os.O_APPEND, 0666)
			if err != nil {
				log.Fatalln("Could not open '%s': %s", profilingFileName, err)
			}

			_, err = fmt.Fprintln(file, fmt.Sprintf(
				"%v, %v, %v, %v, %v",
				globalWorkGroupSize,
				localWorkGroupSize,
				numSpheres,
				float64(profilingLog.forceCompute) / profilingLogLength,
				float64(profilingLog.sphereRender) / profilingLogLength,
			))
			if err != nil {
				log.Fatalln("Could not write to '%s': %s", profilingFileName, err)
			}
			file.Close()
		}
	}
}


func init() {
	runtime.LockOSThread()
	rand.Seed(time.Now().UnixNano())
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


func newGravityShader(localWorkGroupSize, numSpheres uint32) (uint32, error) {
	file, err := os.Open("gravity_compute_shader.glsl")
	if err != nil {
		return 0, fmt.Errorf("Could not open 'gravity_compute_shader.glsl': %s", err)
	}
	defer file.Close()

	fi, err := file.Stat()
	if err != nil {
		return 0, fmt.Errorf("Could not obtain info on 'gravity_compute_shader.glsl': %s", err)
	}

	bSource := make([]byte, fi.Size())
	_, err = file.Read(bSource)
	if err != io.EOF && err != nil {
		return 0, fmt.Errorf("Could not read from 'gravity_compute_shader.glsl': %s", err)
	}

	source := string(bSource) + "\x00"
	source = fmt.Sprintf(source, localWorkGroupSize, numSpheres)

	shader := gl.CreateShader(gl.COMPUTE_SHADER)
	if shader == 0 {
		return 0, fmt.Errorf("Could not create name for shader 'gravity_compute_shader.glsl'!")
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
			"Failed to compile 'gravity_compute_shader.glsl'!\n#>> Source:\n%s\n#>> InfoLog:\n%s",
			source,
			infoLog,
		)
	}

	return shader, nil
}


func newGravityProgram(computeShader uint32) (uint32, error) {
	program := gl.CreateProgram()
	if program == 0 {
		return 0, fmt.Errorf("Could not create name for gravity program!")
	}

	gl.AttachShader(program, computeShader)
	gl.LinkProgram(program)
	gl.DetachShader(program, computeShader)
	gl.ValidateProgram(program)

	var validateStatus int32
	gl.GetProgramiv(program, gl.VALIDATE_STATUS, &validateStatus)

	var linkStatus int32
	gl.GetProgramiv(program, gl.LINK_STATUS, &linkStatus)

	if /*validateStatus == gl.FALSE ||*/ linkStatus == gl.FALSE {
		var infoLogLength int32
		gl.GetProgramiv(program, gl.INFO_LOG_LENGTH, &infoLogLength)

		infoLog := strings.Repeat("\x00", int(infoLogLength + 1))
		gl.GetProgramInfoLog(program, infoLogLength, nil, gl.Str(infoLog))

		gl.DeleteProgram(program)

		return 0, fmt.Errorf(
			"Failed to link gravity program!\n#>> InfoLog:\n%s",
			infoLog,
		)
	}

	return program, nil
}


func newAxisProgram(vertexShader, fragmentShader uint32) (uint32, error) {
	program := gl.CreateProgram()
	if program == 0 {
		return 0, fmt.Errorf("Could not create name for axis program!")
	}

	gl.AttachShader(program, vertexShader)
	gl.AttachShader(program, fragmentShader)
	gl.LinkProgram(program)
	gl.DetachShader(program, vertexShader)
	gl.DetachShader(program, fragmentShader)
	gl.ValidateProgram(program)

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
			"Failed to link axis program!\n#>> InfoLog:\n%s",
			infoLog,
		)
	}

	return program, nil
}


func newSphereProgram(vertexShader, tesselationControlShader, tesselationEvaluationShader, fragmentShader uint32) (uint32, error) {
	program := gl.CreateProgram()
	if program == 0 {
		return 0, fmt.Errorf("Could not create name for sphere program!")
	}

	gl.AttachShader(program, vertexShader)
	gl.AttachShader(program, tesselationControlShader)
	gl.AttachShader(program, tesselationEvaluationShader)
	gl.AttachShader(program, fragmentShader)
	gl.LinkProgram(program)
	gl.DetachShader(program, vertexShader)
	gl.DetachShader(program, tesselationControlShader)
	gl.DetachShader(program, tesselationEvaluationShader)
	gl.DetachShader(program, fragmentShader)
	gl.ValidateProgram(program)

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
			"Failed to link sphere program!\n#>> InfoLog:\n%s",
			infoLog,
		)
	}

	return program, nil
}


func debugSourceString(source uint32) string {
	switch source {
	case gl.DEBUG_SOURCE_API:
		return "API"
	case gl.DEBUG_SOURCE_APPLICATION:
		return "APPLICATION"
	case gl.DEBUG_SOURCE_OTHER:
		return "OTHER"
	case gl.DEBUG_SOURCE_SHADER_COMPILER:
		return "SHADER_COMPILER"
	case gl.DEBUG_SOURCE_THIRD_PARTY:
		return "THIRD_PARTY"
	case gl.DEBUG_SOURCE_WINDOW_SYSTEM:
		return "WINDOW_SYSTEM"
	default:
		return "UNKNOWN"
	}
}


func debugTypeString(gltype uint32) string {
	switch gltype {
	case gl.DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		return "DEPRECATED_BEHAVIOR"
	case gl.DEBUG_TYPE_ERROR:
		return "ERROR"
	case gl.DEBUG_TYPE_MARKER:
		return "MARKER"
	case gl.DEBUG_TYPE_OTHER:
		return "OTHER"
	case gl.DEBUG_TYPE_PERFORMANCE:
		return "PERFORMANCE"
	case gl.DEBUG_TYPE_POP_GROUP:
		return "POP_GROUP"
	case gl.DEBUG_TYPE_PORTABILITY:
		return "PORTABILITY"
	case gl.DEBUG_TYPE_PUSH_GROUP:
		return "PUSH_GROUP"
	case gl.DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		return "UNDEFINED_BEHAVIOR"
	default:
		return "UNKNOWN"
	}
}


func debugSeverityString(severity uint32) string {
	switch severity {
	case gl.DEBUG_SEVERITY_HIGH:
		return "HIGH"
	case gl.DEBUG_SEVERITY_LOW:
		return "LOW"
	case gl.DEBUG_SEVERITY_MEDIUM:
		return "MEDIUM"
	case gl.DEBUG_SEVERITY_NOTIFICATION:
		return "NOTIFICATION"
	default:
		return "UNKNOWN"
	}
}


