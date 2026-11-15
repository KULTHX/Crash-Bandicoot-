#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <GLES3/gl3.h>
#include <android/log.h>
#include <stdbool.h>

#define POC_TAG "SDL_POC"
#define POC_LOGI(...) __android_log_print(ANDROID_LOG_INFO, POC_TAG, __VA_ARGS__)
#define POC_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, POC_TAG, __VA_ARGS__)

static int fail(const char *stage, const char *detail)
{
    POC_LOGE("FAIL at %s: %s", stage, detail ? detail : "unknown error");
    return 1;
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    POC_LOGI("Starting SDL3 Android host test");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        return fail("SDL_Init", SDL_GetError());
    }
    if (!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES) ||
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) ||
        !SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0)) {
        const char *error = SDL_GetError();
        SDL_Quit();
        return fail("SDL_GL_SetAttribute", error);
    }

    SDL_Window *window = SDL_CreateWindow(
        "NIZAR CRASH 3 - SDL3 Android POC", 960, 540,
        SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window == NULL) {
        const char *error = SDL_GetError();
        SDL_Quit();
        return fail("SDL_CreateWindow", error);
    }

    SDL_GLContext context = SDL_GL_CreateContext(window);
    if (context == NULL) {
        const char *error = SDL_GetError();
        SDL_DestroyWindow(window);
        SDL_Quit();
        return fail("SDL_GL_CreateContext", error);
    }
    if (!SDL_GL_MakeCurrent(window, context)) {
        const char *error = SDL_GetError();
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return fail("SDL_GL_MakeCurrent", error);
    }

    const GLubyte *gl_version = glGetString(GL_VERSION);
    const GLubyte *gl_renderer = glGetString(GL_RENDERER);
    const GLubyte *gl_vendor = glGetString(GL_VENDOR);
    if (gl_version == NULL || gl_renderer == NULL) {
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return fail("OpenGL ES query", "glGetString returned NULL");
    }

    POC_LOGI("SDL platform: %s", SDL_GetPlatform());
    POC_LOGI("OpenGL ES version: %s", (const char *)gl_version);
    POC_LOGI("GPU renderer: %s", (const char *)gl_renderer);
    POC_LOGI("GPU vendor: %s", gl_vendor ? (const char *)gl_vendor : "unknown");

    int drawable_width = 0;
    int drawable_height = 0;
    SDL_GetWindowSizeInPixels(window, &drawable_width, &drawable_height);
    if (drawable_width <= 0 || drawable_height <= 0) {
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return fail("SDL_GetWindowSizeInPixels", "invalid drawable dimensions");
    }

    glViewport(0, 0, drawable_width, drawable_height);
    const Uint64 end_ticks = SDL_GetTicks() + 6000;
    bool quit = false;
    unsigned int frames = 0;

    while (!quit && SDL_GetTicks() < end_ticks) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                quit = true;
            }
        }
        glViewport(0, 0, drawable_width, drawable_height);
        glClearColor(0.08f, 0.18f, 0.32f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        SDL_GL_SwapWindow(window);
        ++frames;
        SDL_Delay(16);
    }

    const GLenum gl_error = glGetError();
    if (gl_error != GL_NO_ERROR || frames < 10) {
        POC_LOGE("Render loop failed: glError=0x%x frames=%u", gl_error, frames);
        SDL_GL_DestroyContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 2;
    }

    POC_LOGI("PASS: SDL3 Android OpenGL ES 3 context and swap loop (%u frames)", frames);
    SDL_GL_DestroyContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
