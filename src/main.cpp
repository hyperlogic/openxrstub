// openxr example


#define XR_USE_GRAPHICS_API_OPENGL
#if defined(WIN32)
#define XR_USE_PLATFORM_WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#elif defined(__ANDROID__)
#define XR_USE_PLATFORM_ANDROID
#else
#define XR_USE_PLATFORM_XLIB
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#include <stdlib.h> //rand()

static bool quitting = false;
static float r = 0.0f;
static SDL_Window *window = NULL;
static SDL_GLContext gl_context;
static SDL_Renderer *renderer = NULL;

void render()
{
    SDL_GL_MakeCurrent(window, gl_context);
    r = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);

    glClearColor(r, 0.4f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    SDL_GL_SwapWindow(window);
}


int SDLCALL watch(void *userdata, SDL_Event* event)
{
    if (event->type == SDL_APP_WILLENTERBACKGROUND)
    {
        quitting = true;
    }

    return 1;
}

int main(int argc, char *argv[])
{
    XrResult result;
    uint32_t extensionCount = 0;
    result = xrEnumerateInstanceExtensionProperties(NULL, 0, &extensionCount, NULL);
    if (!XR_SUCCEEDED(result))
    {
        printf("xrEnumerateInstanceExtensionProperties failed %d", result);
        return 1;
    }

    printf("Runtime supports %d extensions\n", extensionCount);

    std::vector<XrExtensionProperties> extensionProperties(extensionCount);
    for (uint32_t i = 0; i < extensionCount; i++) {
        extensionProperties[i].type = XR_TYPE_EXTENSION_PROPERTIES;
        extensionProperties[i].next = NULL;
    }
    result = xrEnumerateInstanceExtensionProperties(NULL, extensionCount, &extensionCount, extensionProperties.data());
    if (!XR_SUCCEEDED(result))
    {
        printf("xrEnumerateInstanceExtensionProperties failed %d\n", result);
        return 1;
    }

    bool glSupported = false;
    printf("extensions:\n");
    for (uint32_t i = 0; i < extensionCount; ++i) {
        if (!strcmp(XR_KHR_OPENGL_ENABLE_EXTENSION_NAME, extensionProperties[i].extensionName))
        {
            glSupported = true;
        }
        printf("extensions: %s\n", extensionProperties[i].extensionName);
    }

    if (!glSupported)
    {
        printf("XR_KHR_OPENGL_ENABLE_EXTENSION not supported!\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_EVENTS) != 0)
    {
        SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow("title", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 512, 512, SDL_WINDOW_OPENGL);

    gl_context = SDL_GL_CreateContext(window);

        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
        if (!renderer)
        {
            SDL_Log("Failed to SDL Renderer: %s", SDL_GetError());
            return -1;
	}

    SDL_AddEventWatch(watch, NULL);

    while (!quitting)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event) )
        {
            if (event.type == SDL_QUIT)
            {
                quitting = true;
            }
        }

        render();
        SDL_Delay(2);
    }

    SDL_DelEventWatch(watch, NULL);
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
