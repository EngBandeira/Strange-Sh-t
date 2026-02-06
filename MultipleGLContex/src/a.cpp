
#include <cstddef>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <bits/stdc++.h>
#include <stdio.h>
#include <sched.h>
#include <sys/mman.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "vendor/glad/glad.h"
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS


#ifndef CHILD_SIG
#define CHILD_SIG SIGUSR1       /* Signal to be generated on termination
                                   of cloned child */
#endif


#include "vendor/imgui/imgui.h"
#include "vendor/imgui/imgui_impl_glfw.h"
#include "vendor/imgui/imgui_impl_opengl3.h"

#include "common.hpp"


unsigned int tex_post, post_program;

void init();



struct ChildParams {
    unsigned char c[2048];
};


void post_processing() {
    post_program = glCreateProgram();

    char *file = utils.read_file ("src/post.comp", nullptr);
    if(file == NULL)
        logger.sendError("file");
    unsigned int compiled_shader = glCreateShader (GL_COMPUTE_SHADER);
    glShaderSource (compiled_shader, 1, &file, NULL);//aq
    glCompileShader (compiled_shader);

    free (file);

    utils.get_shader_status(compiled_shader, GL_COMPILE_STATUS);

    glAttachShader (post_program, compiled_shader);

    glLinkProgram(post_program);
    utils.get_program_status(post_program,GL_LINK_STATUS);
    glUseProgram(post_program);
    glBindImageTexture(0, tex_post, 0, GL_TRUE, 0, GL_READ_WRITE,  GL_RGBA8 );
    glDispatchCompute(120,68,1);
    glMemoryBarrier(GL_TEXTURE_FETCH_BARRIER_BIT);
    glDeleteProgram(post_program);
}


static int              /* Startup function for cloned child */
childFunc(void *arg)
{
    printf("Child:  PID=%ld PPID=%ld\n", (long) getpid(), (long) getppid());

    ChildParams *cp = (ChildParams*)arg;

    printf("CHILDAAAA\n");

    for(int i = 0; i < 256; i++) {
        printf("%d",cp->c[i]);
    }
    printf("\n");

    /* The following changes may affect parent */
    glfwMakeContextCurrent((GLFWwindow*)(cp->c));
    post_processing();


    return 22;      /* Child terminates now */
}

static void             /* Handler for child termination signal */
grimReaper(int sig)
{
    /* UNSAFE: This handler uses non-async-signal-safe functions
       (printf(), strsignal(); see Section 21.1.2) */

    int savedErrno = errno;             /* In case we change 'errno' here */

    printf("Caught signal %d (%s)\n", sig, strsignal(sig));

    errno = savedErrno;
}


void setTexParameter(unsigned int tex_type){
    glTexParameteri(tex_type, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(tex_type, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(tex_type, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(tex_type ,GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(tex_type, GL_TEXTURE_BASE_LEVEL, 0);
    glTexParameteri(tex_type, GL_TEXTURE_MAX_LEVEL, LEVEL);
}

void error_callback(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
    if( type == GL_DEBUG_TYPE_ERROR ) {
        fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = "
                "0x%x, message = %s\n",
                (type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
                type,
                severity,
                message
        );
    }
}

int
main(int argc, char *argv[])
{
    setenv("XDG_SESSION_TYPE", "x11" , 1); // THIS IN NECESSARY TO USE THE RENDERDOC, IT DONT RUNS ON WAYLAND
    glfwInit();
    glfwWindowHint(GLFW_ALPHA_BITS, 8);    // if (gl_GlobalInvocationID.x > 1080 || gl_GlobalInvocationID.y > 1080)

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

    GLFWwindow *glfw_window = glfwCreateWindow(1920, 1080, "PhysicsEngine", NULL, NULL);
    printf("Pai %ld\n",(void*)glfw_window);
    if( glfw_window == NULL ) {
        glfwTerminate();
        logger.sendError("failed to crate a Glfw Window\n", 1);
    }

    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);
    glfwSetFramebufferSizeCallback(glfw_window,
        [](GLFWwindow *window, int width, int height) {
            glViewport(0, 0, width, height);
        });

    if(! gladLoadGLLoader((GLADloadproc)glfwGetProcAddress) ) {
        logger.sendError("cannot init glad", 1);
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDebugMessageCallback(error_callback, 0);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    glfwMakeContextCurrent(glfw_window);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.IniFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(glfw_window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    glfwSetKeyCallback(glfw_window, ImGui_ImplGlfw_KeyCallback);



    glGenTextures(1, &tex_post);
    glBindTexture(GL_TEXTURE_2D, tex_post);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1920, 1080, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

    setTexParameter(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);


    glClearColor(.2, .20, .20, .5);

    printf("Parent: PID=%ld PPID=%ld\n", (long) getpid(), (long) getppid());

    /* Set up an argument structure to be passed to cloned child, and
       set some process attributes that will be modified by child */

    ChildParams cp;                     /* Passed to child function */
    memcpy(cp.c, glfw_window, 2048);

    printf("pIBTYAFGFVBSVG\n");
    unsigned char *cckk = (unsigned char*)glfw_window;
    for(int i = 0; i < 256; i++) {
        printf("%d",cckk[i]);
    }
    printf("\n");

    const int STACK_SIZE = 65536;

    char *stack = (char*)mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    if (stack == MAP_FAILED)
        logger.sendError("mmap");
    char *stackTop = stack + STACK_SIZE;  /* Assume stack grows downward */
    /* Establish handler to catch child termination signal */

    struct sigaction sa;
    if (CHILD_SIG != 0) {
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        sa.sa_handler = grimReaper;
        if (sigaction(CHILD_SIG, &sa, NULL) == -1)
            logger.sendError("sigaction");
    }

    /* Create child; child commences execution in childFunc() */

    if (clone(childFunc, stackTop, flags | CHILD_SIG, &cp) == -1)
        logger.sendError("clone");

    /* Now that child has been created, we can deallocate the stack */

    munmap(stack, STACK_SIZE);

    /* Parent falls through to here. Wait for child; __WCLONE option is
       required for child notifying with signal other than SIGCHLD. */

    int status;
    pid_t pid = waitpid(-1, &status, (CHILD_SIG != SIGCHLD) ? __WCLONE : 0);
    if (pid == -1)
        logger.sendError("waitpid");

    printf("    Child PID=%ld\n", (long) pid);

    // while(!glfwWindowShouldClose(glfw_window)) {
    //     glClear(GL_COLOR_BUFFER_BIT);
    //     ImGui_ImplGlfw_NewFrame();
    //     ImGui_ImplOpenGL3_NewFrame();
    //     ImGui::NewFrame();

    //     ImGui::SetNextWindowSize(ImVec2(500,500));
    //     if( ImGui::Begin("Game", nullptr, ImGuiWindowFlags_None) ) {
    //         ImGui::GetWindowDrawList()->AddImage((ImTextureRef)tex_post,
    //                                             ImGui::GetWindowPos(),
    //                                             ImGui::GetWindowSize() + ImGui::GetWindowPos(),
    //                                             ImVec2(0, 1),
    //                                             ImVec2(1, 0));
    //     }
    //     ImGui::End();


    //     glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    //     glBindBuffer(GL_ARRAY_BUFFER, 0);
    //     ImGui::Render();
    //     ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    //     glfwSwapBuffers(glfw_window);
    //     glfwPollEvents();
    //     flags  = flags & ~(1&2);
    // }

    exit(EXIT_SUCCESS);
}
