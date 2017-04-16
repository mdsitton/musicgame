#include "config.hpp"
#include "game.hpp"

#include <iostream>
#include <stdexcept>

#include "vfs.hpp"
namespace ORGame
{
    GameManager::GameManager()
    :m_width(800),
    m_height(600),
    m_fullscreen(false),
    m_title("OpenRhythm"),
    m_window(m_width, m_height, m_fullscreen, m_title),
    m_context(3, 2, 0),
    m_eventManager(),
    m_eventPump(&m_eventManager),
    m_song("/data/songs/testsong")
    {
        m_running = true;

        m_logger = spdlog::get("default");


        m_window.make_current(&m_context);

        m_window.disable_sync();

        //VFS.AddLoader(new ttvfs::DiskLoader);

        //
        // AppPath gets mounted on osx
        // BasePath gets mounted and overwrites any files similar to those in AppPath (data)
        // HomePath gets mounted and overwrites any files similar to those in BasePath (configs)
        // std::cout << ORCore::GetBasePath() << std::endl;
    #if OSX_APP_BUNDLE
        ORCore::mount(ORCore::GetAppPath(), "");
    #endif
        ORCore::mount(ORCore::GetBasePath(), "/bob");
        std::vector<std::string> paths = ORCore::resolveSystemPath("/bob");
        for (auto &i: paths) {
            auto bob = ORCore::sysGetPathContents(i);
            std::cout << i << " " << bob.size() << std::endl;
        }
        //VFS.Mount( ORCore::GetBasePath().c_str(), "" );
        //VFS.Mount( ORCore::GetHomePath().c_str(), "" );

        //ORCore::mount( "./data", "data" );
        m_song.load();

        //std::cout << "Song: " << (m_song.length() / 1000) / 60 << " minutes long" << std::endl;

        //ORCore::Track *track = m_song.getTrack( ORCore::TrackType::Guitar, ORCore::Difficulty::Expert );
        //std::cout << "Song: loaded track for " << ORCore::TrackNameForType( track->info().type ) << std::endl;

        m_tempoTrack = m_song.get_tempo_track();

        m_song.load_tracks();
        m_playerTrack = &(*m_song.get_tracks())[0];

        if(!gladLoadGL())
        {
            throw std::runtime_error(_("Error: GLAD failed to load."));
        }

        m_lis.handler = std::bind(&GameManager::event_handler, this, std::placeholders::_1);
        m_lis.mask = ORCore::EventType::EventAll;


        m_eventManager.add_listener(m_lis);

        m_fpsTime = 0.0;

        m_ss = std::cout.precision();

        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        ORCore::ShaderInfo vertInfo {GL_VERTEX_SHADER, "./data/shaders/main.vs"};
        ORCore::ShaderInfo fragInfo {GL_FRAGMENT_SHADER, "./data/shaders/main.fs"};

        ORCore::Shader vertShader(vertInfo);
        ORCore::Shader fragShader(fragInfo);

        m_program = std::make_unique<ORCore::ShaderProgram>(&vertShader, &fragShader);

        m_program->check_error();
        m_program->use();
        m_orthoID = m_program->uniform_attribute("ortho");

        m_texture = std::make_unique<ORCore::Texture>(GL_TEXTURE_2D);
        ORCore::Image img = ORCore::loadSTB("data/blank.png");
        m_texture->update_image_data(img);
        m_renderer = std::make_unique<ORCore::Render>(m_program.get(), m_texture.get());

        resize(m_width, m_height);

        prep_render_bars();
        prep_render_notes();
        m_renderer->mesh_commit();

        m_program->set_uniform(m_orthoID, m_ortho);

        glClearColor(0.5, 0.5, 0.5, 1.0);
    }

    GameManager::~GameManager()
    {

        glDeleteVertexArrays(1, &m_vao);
        m_window.make_current(nullptr);

    }

    void GameManager::prep_render_bars()
    {

        std::vector<TempoTrackEvent> bars = m_tempoTrack->get_events(0.0, m_song.length(), ORGame::EventType::Bar);
        std::cout << "Bar Count: " << bars.size() << " Song Length: " << m_song.length() << std::endl;
        for (size_t i = 0; i < bars.size(); i++) {
            float z = bars[i].bar->time / 3.0f;

            ORCore::Mesh mesh = {
                glm::vec3{512.0f, 4.0f, 0.0f},                  // Scale
                glm::vec3{(m_width/2.0f)-256, z, 0.0f},   // Position - center the line on the screen
                2,                                              // Values per vert
                ORCore::create_rect_mesh(glm::vec4{1.0,1.0,1.0,1.0}),                     // Create the rect mesh.
            };

            m_renderer->add_mesh(mesh);
        }
    }

    void GameManager::prep_render_notes()
    {
        std::vector<TrackNote*> notes = m_playerTrack->get_notes_in_frame(0.0, m_song.length());
        std::cout << "Note Count: " << notes.size() << std::endl;
        for (size_t i = 0; i < notes.size(); i++) {
            float z = notes[i]->time / 3.0f;
            glm::vec4 color;
            if( notes[i]->type == NoteType::Green) {
                color = glm::vec4{0.0,1.0,0.0,1.0};
            } else if( notes[i]->type == NoteType::Red) {
                color = glm::vec4{1.0,0.0,0.0,1.0};
            } else if( notes[i]->type == NoteType::Yellow) {
                color = glm::vec4{1.0,1.0,0.0,1.0};
            } else if( notes[i]->type == NoteType::Blue) {
                color = glm::vec4{0.0,0.0,1.0,1.0};
            } else if( notes[i]->type == NoteType::Orange) {
                color = glm::vec4{1.0,0.5,0.0,1.0};
            }
            //std::cout << "Length: " << notes[i]->length/3.0f << std::endl;

            ORCore::Mesh mesh = {
                glm::vec3{30.0f, 4.0, 0.0f},                                               // Scale
                glm::vec3{static_cast<int>(notes[i]->type)*40, z, 0.0f},   // Position - center the line on the screen
                2,                                                                          // Values per vert
                ORCore::create_rect_mesh(color),                                                 // Create the rect mesh.
            };

            m_renderer->add_mesh(mesh);

            float noteLength = notes[i]->length/3.0f;
            if (noteLength > 4.0f)
            {
                ORCore::Mesh mesh = {
                    glm::vec3{10.0f, noteLength, 0.0f},                                               // Scale
                    glm::vec3{10+(static_cast<int>(notes[i]->type)*40), z, 0.0f},   // Position - center the line on the screen
                    2,                                                                          // Values per vert
                    ORCore::create_rect_mesh(color),                                                 // Create the rect mesh.
                };

                m_renderer->add_mesh(mesh);
            }
        }
    }

    void GameManager::start()
    {
        GLenum error;
        while (m_running)
        {
            m_fpsTime += m_clock.tick();
            m_eventPump.process();

            update();
            render();

            do
            {
                error = glGetError();
                if (error != GL_NO_ERROR)
                {
                    std::cout << error << std::endl;
                }
            } while(error != GL_NO_ERROR);

            m_window.flip();
            if (m_fpsTime >= 2000.0) {
                std::cout.precision (5);
                std::cout << "FPS: " << m_clock.get_fps() << std::endl;
                std::cout << "Song Time: " << m_songTime << std::endl;
                std::cout.precision (m_ss);
                m_fpsTime = 0;
            }
        }
    }

    void GameManager::resize(int width, int height)
    {
        m_width = width;
        m_height = height;
        glViewport(0, 0, m_width, m_height);
        m_ortho = glm::ortho(0.0f, static_cast<float>(m_width), static_cast<float>(m_height), 0.0f, -1.0f, 1.0f);
    }

    bool GameManager::event_handler(const ORCore::Event &event)
    {
        switch(event.type) {
            case ORCore::Quit: {
                m_running = false;
                break;
            }
            case ORCore::MouseMove: {
                auto ev = ORCore::event_cast<ORCore::MouseMoveEvent>(event);
                //std::cout << "mouse x: " << ev.x << " mouse y" << ev.y << std::endl;
                m_mouseX = ev.x;
                m_mouseY = ev.y;
                break;
            }
            case ORCore::WindowSize: {
                auto ev = ORCore::event_cast<ORCore::WindowSizeEvent>(event);
                resize(ev.width, ev.height);
                break;
            }
            case ORCore::KeyDown: {
                auto ev = ORCore::event_cast<ORCore::KeyDownEvent>(event);
                switch(ev.key) {
                    case ORCore::KeyCode::KEY_F:
                        std::cout << "Key F" << std::endl;
                        break;
                    default:
                        std::cout << "Other Key" << std::endl;
                        break;
                }
            }
            default:
                break;
        }
        return true;
    }

    void GameManager::handle_song()
    {
    }

    void GameManager::update()
    {
        m_songTime = m_clock.get_current_time();
        m_program->set_uniform(m_orthoID,
            glm::translate(m_ortho, glm::vec3(0.0f, (-m_songTime)/3.0f, 0.0f))); // translate projection with song
    }

    void GameManager::render()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_program->use();
        m_renderer->render();


    }
}