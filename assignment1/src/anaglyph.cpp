#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <render/shader.h>
#include <render/texture.h>
#include <models/box.h>

#include <vector>
#include <iostream>
#define _USE_MATH_DEFINES
#include <math.h>

static GLFWwindow *window;
static int windowWidth = 1024; 
static int windowHeight = 768;

static void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);

// OpenGL camera view parameters
static glm::vec3 originalEyeCenter(0, 0, 100);

static glm::vec3 eyeCenter = originalEyeCenter;
static glm::vec3 lookat(0, 0, 0);
static glm::vec3 up(0, 1, 0);

static glm::float32 FoV = 45;
static glm::float32 zNear = 0.1f; 
static glm::float32 zFar = 1000.0f;

// View control 
static float viewAzimuth = M_PI / 2;
static float viewPolar = M_PI / 2;
static float viewDistance = 100.0f;
static bool rotating = false;

// Scene control 
static int numBoxes = 1;				// Debug: set numBoxes to 1.
std::vector<glm::mat4> boxTransforms;	// We represent the scene by a single box and a number of transforms for drawing the box at different locations.

//anaglyph control
static float ipd = 0.65f;
static float convergenceDistance = 15.0f;














enum AnaglyphMode {
	None,
	ToeIn, 
	Asymmetric, 
	AnaglyphModeCount,
};

static std::string strAnaglyphMode[] = {
	"None", 
	"Toe-in", 
	"Asymmetric view frustum", 
	"Invalid",
};

static AnaglyphMode anaglyphMode = AnaglyphMode::None;

// Helper functions 

static void nextAnaglyphMode() {
	anaglyphMode = (AnaglyphMode)(((int)anaglyphMode + 1) % (int)AnaglyphModeCount);
}

static int randomInt() {
	return rand();
}

static float randomFloat() {
	float r = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
	return r;
}

static glm::vec3 randomVec3() {
	return glm::vec3(randomFloat(), randomFloat(), randomFloat());
}

static void generateScene() {
	boxTransforms.clear();
	if (numBoxes == 1) {
		// Use this for debugging
		glm::mat4 modelMatrix = glm::mat4();
		modelMatrix = glm::translate(modelMatrix, glm::vec3(0, 0, 0));
		modelMatrix = glm::scale(modelMatrix, glm::vec3(16, 16, 16));
		boxTransforms.push_back(modelMatrix);
	} else {
        //scene stuff
        //creates 6x6 grid for boxes to generate on
        for (int z = -6; z <= 6; ++z) {
            for (int x = -6; x <= 6; ++x) {
                //positons are 8f by 8f apart
                glm::vec3 position(x * 8.0f, 0, z * 8.0f);

                //randomly scales the box between 1 and 5
                float scaling = (1 + (randomInt() % 4)) * 1.0f;
                //uniform scaling
                glm::vec3 scale(scaling, scaling, scaling);

                //randomly rotates the box
                float angle = randomFloat() * M_PI * 2;
                glm::vec3 axis = glm::normalize(randomVec3() - 0.5f);


                glm::mat4 modelMatrix = glm::mat4();
                //puts box into position
                modelMatrix = glm::translate(modelMatrix, position);
                //rotates the box
                modelMatrix = glm::rotate(modelMatrix, angle, axis);
                //scales the box
                modelMatrix = glm::scale(modelMatrix, scale);

                boxTransforms.push_back(modelMatrix);
            }
        }

	}
}

// Debugging functions 

static void printAnaglyphMode() {
	std::cout << "Anaglyph mode: " << strAnaglyphMode[(int)anaglyphMode] << std::endl;
}

static void printVec3(glm::vec3 v) {
	std::cout << v.x << " " << v.y << " " << v.z << std::endl;
}

static void printMat4(glm::mat4 m) {
	// Column major
	std::cout << m[0][0] << " " << m[1][0] << " " << m[2][0] << " " << m[3][0] << std::endl;
	std::cout << m[0][1] << " " << m[1][1] << " " << m[2][1] << " " << m[3][1] << std::endl;
	std::cout << m[0][2] << " " << m[1][2] << " " << m[2][2] << " " << m[3][2] << std::endl;
	std::cout << m[0][3] << " " << m[1][3] << " " << m[2][3] << " " << m[3][3] << std::endl;
}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		std::cerr << "Failed to initialize GLFW." << std::endl;
		return -1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // For MacOS
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(windowWidth, windowHeight, "Anaglyph Rendering", NULL, NULL);
	if (window == NULL)
	{
		std::cerr << "Failed to open a GLFW window." << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetKeyCallback(window, key_callback);

	// Ensure we can capture mouse cursor movement 
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// Load OpenGL functions, gladLoadGL returns the loaded version, 0 on error.
	int version = gladLoadGL(glfwGetProcAddress);
	if (version == 0)
	{
		std::cerr << "Failed to initialize OpenGL context." << std::endl;
		return -1;
	}

	srand(2024);

	// Background
    //glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
	glClearColor(163 / 255.0f, 227 / 255.0f, 255 / 255.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Create a box
	Box box;
	box.initialize();

	// Create the scene with a set of boxes represented by their transforms
	generateScene();

	// Set a perspective camera 
	glm::mat4 projectionMatrix = glm::perspective(glm::radians(FoV), (float)windowWidth / windowHeight, zNear, zFar);

	printAnaglyphMode();
	do
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// TODO: Render anaglyph 
		// --------------------------------------------------------------------

		if (anaglyphMode == None) {
			// Clear the screen
			glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Set camera view matrix 
			glm::mat4 viewMatrix = glm::lookAt(eyeCenter, lookat, up);
			glm::mat4 vp = projectionMatrix * viewMatrix;
			
			// Draw 
			for (int i = 0; i < numBoxes; ++i) {
				box.render(vp, boxTransforms[i]);
			}

		} else {
			
			glm::mat4 vpLeft;
			glm::mat4 vpRight;
            glm::vec3 leftEyePosition, rightEyePosition;
            glm::vec3 convergencePoint;
            //asymmetric view crap
            float aspectRatio = float(windowWidth) / float(windowHeight);
            //rendering distance crap
            float nearPlane = 15.0f;
            float farPlane = 200.0f;
            //fov of asymmetric
            float fovY = glm::radians(75.0f);
            float halfWidth = tan(fovY / 2) * nearPlane;
            //ipd
            float halfIPD = (ipd/ 2.0f);
            //frustum crap
            float left = -aspectRatio * halfWidth - (ipd/ 2.0f);
            float right = aspectRatio * halfWidth - (ipd/ 2.0f);
            float bottom = -halfWidth;
            float top = halfWidth;

			if (anaglyphMode == ToeIn) {

                leftEyePosition = eyeCenter - glm::vec3((halfIPD*10), 0.0f, 0.0f);
                rightEyePosition = eyeCenter + glm::vec3((halfIPD*10), 0.0f, 0.0f);
                convergencePoint = lookat - glm::vec3(0.0f, 0.0f, convergenceDistance);

                glm::mat4 viewMatrixLeft = glm::lookAt(leftEyePosition, convergencePoint,  glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 viewMatrixRight = glm::lookAt(rightEyePosition, convergencePoint,  glm::vec3(0.0f, 1.0f, 0.0f));

                vpLeft = projectionMatrix * viewMatrixLeft;
                vpRight = projectionMatrix * viewMatrixRight;


            } else if (anaglyphMode == Asymmetric) {

				// TODO: Implement the asymmetric view frustum here
				// ------------------------------------------------------------


                glm::mat4 projectionLeft = glm::frustum(left, right, bottom, top, nearPlane, farPlane);


                //used halfipd/2 as i thought it looked better than just halfipd for asymmetric view,
                left = -aspectRatio * halfWidth + (halfIPD/2.0f);
                right = aspectRatio * halfWidth + (halfIPD/2.0f);
                glm::mat4 projectionRight = glm::frustum(left, right, bottom, top, nearPlane, farPlane);

                leftEyePosition = eyeCenter - glm::vec3((halfIPD/2.0f), 0.0f, 0.0f);
                rightEyePosition = eyeCenter + glm::vec3((halfIPD/2.0f), 0.0f, 0.0f);

                glm::mat4 viewMatrixLeft = glm::lookAt(leftEyePosition, lookat, up);
                glm::mat4 viewMatrixRight = glm::lookAt(rightEyePosition, lookat, up);

                vpLeft = projectionLeft * viewMatrixLeft;
                vpRight = projectionRight * viewMatrixRight;

			}

			// TODO: Implement two-pass rendering to draw the anaglyph
			// ----------------------------------------------------------------

            //renders boxes using asymmetric and toe-in
            glColorMask(GL_TRUE, GL_FALSE, GL_FALSE, GL_TRUE);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            for (int i = 0; i < numBoxes; ++i) {
                box.render(vpLeft, boxTransforms[i]);
            }

            glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
            glClear(GL_DEPTH_BUFFER_BIT);
            for (int i = 0; i < numBoxes; ++i) {
                box.render(vpRight, boxTransforms[i]);
            }

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        }

		// --------------------------------------------------------------------


		// Animation
		static double lastTime = glfwGetTime();
		double currentTime = glfwGetTime();
		float deltaTime = float(currentTime - lastTime);
		lastTime = currentTime;
		if (rotating) {
			viewAzimuth += 1.0f * deltaTime;
			eyeCenter.x = viewDistance * cos(viewAzimuth);
			eyeCenter.z = viewDistance * sin(viewAzimuth);
		}

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (!glfwWindowShouldClose(window));

	// Clean up
	box.cleanup();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
	{
		std::cout << "Space key is pressed." << std::endl;
		rotating = !rotating;
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		std::cout << "Reset." << std::endl;
		rotating = false;
		eyeCenter = originalEyeCenter;
		viewAzimuth = M_PI / 2;
		viewPolar = M_PI / 2;
	}

	if (key == GLFW_KEY_UP && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewPolar -= 0.1f;
		eyeCenter.y = viewDistance * cos(viewPolar);
	}

	if (key == GLFW_KEY_DOWN && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewPolar += 0.1f;
		eyeCenter.y = viewDistance * cos(viewPolar);
	}

	if (key == GLFW_KEY_LEFT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewAzimuth -= 0.1f;
		eyeCenter.x = viewDistance * cos(viewAzimuth);
		eyeCenter.z = viewDistance * sin(viewAzimuth);
	}

	if (key == GLFW_KEY_RIGHT && (action == GLFW_REPEAT || action == GLFW_PRESS))
	{
		viewAzimuth += 0.1f;
		eyeCenter.x = viewDistance * cos(viewAzimuth);
		eyeCenter.z = viewDistance * sin(viewAzimuth);
	}

	if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        nextAnaglyphMode(); 
		printAnaglyphMode();
	}

	// Adjust the IPD value to match your actual viewing distance
	// Special case: IPD == 0 means no 3D effect.

	if (key == GLFW_KEY_COMMA) {
		ipd -= 0.1f;
		ipd = std::max(ipd, 0.0f);
		std::cout << "IPD: " << ipd << std::endl;
	}

	if (key == GLFW_KEY_PERIOD) {
		ipd += 0.1f;
		std::cout << "IPD: " << ipd << std::endl;
	}

	if (key == GLFW_KEY_1) {
		numBoxes = 1;
		generateScene();
	}

	if (key == GLFW_KEY_0) {
		numBoxes = 100;
		generateScene();
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
	// Optionally, you can implement your own mouse support.
}
