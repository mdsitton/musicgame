#include "window.hpp"

namespace ORCore
{

    void init_video()
    {
        SDL_Init(0);
        SDL_InitSubSystem(SDL_INIT_VIDEO);
    }

    Window::Window(int width, int height, bool fullscreen, std::string title)
    : m_width(width), m_height(height), m_fullscreen(fullscreen), m_title(title)
    {
    // TODO - Implement fullscreen mode
        m_x = SDL_WINDOWPOS_CENTERED;
        m_y = SDL_WINDOWPOS_CENTERED;

        m_sdlFlags = defaultWindowFlag;

        m_sdlWindow = SDL_CreateWindow(m_title.c_str(), m_x, m_y, m_width, m_height, m_sdlFlags);
        m_context = nullptr;
    }

    Window::~Window()
    {
        SDL_DestroyWindow(m_sdlWindow);
    }


    SDL_Window* Window::get_platform_window()
    {
        return m_sdlWindow;
    }

    void Window::make_current(Context* context)
    {

        SDL_GLContext pCon;
        SDL_Window* pWin;
        if (context != nullptr) {
            context->set_window(this);
            m_context = context;
            pCon = m_context->get_platform_context();
            pWin = m_sdlWindow;

        } else {
            if (m_context != nullptr) {
                m_context->set_window(nullptr);
            }
            pCon = nullptr;
            pWin = nullptr;
            m_context = nullptr;
        }
        SDL_GL_MakeCurrent(pWin, pCon);
    }

    void Window::flip()
    {
        SDL_GL_SwapWindow(m_sdlWindow);
    }
}
