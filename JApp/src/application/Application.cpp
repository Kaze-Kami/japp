﻿#include "Application.h"

#include <iostream>
#include <chrono>

#include "macros.h"
#include "eventSystem/EventSystem.h"

#include "logger/Log.h"

#define NOW std::chrono::steady_clock::now()

namespace JApp {
	
	Application::Application() {
		/* Initialize the GLFW */
		if (!glfwInit()) {
			APP_CORE_CRITICAL("Can not initialize GLFW!");
			exit(-1);
		}

		/* open gl hints */
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

		/* anti aliasing */
		#if !APP_DEBUG
		APP_CORE_INFO("Using 8 aa-samples");
		glfwWindowHint(GLFW_SAMPLES, 8);
		#endif

		/* window size and hints */
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);

		glfwWindowHint(GLFW_VISIBLE, 0);

		#if APP_DEBUG
		const float scale = .5;
		m_windowWidth = scale * videoMode->width;
		m_windowHeight = scale * videoMode->height;
		APP_CORE_INFO("Creating debug window w={.0f, h={.0f}", m_windowWidth, m_windowHeight);
		m_targetFrameRate = float(videoMode->refreshRate);
		#else
		m_windowWidth = float(videoMode->width);
		m_windowHeight = float(videoMode->height);
		glfwWindowHint(GLFW_MAXIMIZED, 1);
		glfwWindowHint(GLFW_DECORATED, 0);
		m_targetFrameRate = float(videoMode->refreshRate) * 2;
		#if APP_RELEASE
		APP_CORE_INFO("Creating release window w={.0f}, h={.0f} (windowed fullscreen, maximized, not decorated)\n", m_windowWidth, m_windowHeight);
		#elif APP_DIST
		APP_CORE_INFO("Creating dist window w={.0f}, h={.0f} (windowed fullscreen, maximized, not decorated)\n", m_windowWidth, m_windowHeight);
		glfwWindowHint(GLFW_RESIZABLE, 0);
		#endif
		#endif

		m_timeStep = 1.f / m_targetFrameRate;

		/* initialize window */
		m_window = glfwCreateWindow(int(m_windowWidth), int(m_windowHeight), "", nullptr, nullptr);
		if (!m_window) {
			APP_CORE_CRITICAL("Can not create window!");
			glfwTerminate();
			exit(-1);
		}

		/* window position */
		# if APP_DEBUG
		// p = (screen_size - window_size) / 2, (screen_size - screen_size * scale) / 2 = screen_size * (1 - scale) / 2
		const float f = (1.f - scale) / 2;
		const auto px = int(m_windowWidth * f);
		const auto py = int(m_windowHeight * f);

		glfwSetWindowPos(m_window, px, py);
		#endif

		/* Make the window's context current */
		glfwMakeContextCurrent(m_window);

		/* set vsync */
		#if APP_DIST || APP_RELEASE
		APP_CORE_INFO("Vsync on");
		glfwSwapInterval(1);
		#else
		APP_CORE_INFO("Vsync off");
		glfwSwapInterval(0);
		#endif
		APP_CORE_INFO("Target frame rate: {.2f}, update time step: {.4f}", m_targetFrameRate, m_timeStep);

		/* Initialize GLEW */
		glewExperimental = GL_TRUE;
		if (glewInit() != GLEW_OK) {
			APP_CORE_CRITICAL("Can not initialize GLEW!");
			glfwTerminate();
			exit(-1);
		}

		/* enable anti aliasing in open gl */
		#if (!APP_DEBUG)
		GL_CALL(glEnable(GL_MULTISAMPLE));
		#endif

		/* Init event system */
		EventSystem::setWindow(m_window);

		/* register event listeners */
		EventSystem::registerListener(static_cast<EventListener<KeyPressEvent>*>(this));
		EventSystem::registerListener(static_cast<EventListener<ResizeEvent>*>(this));
	}

	Application::~Application() {
		glfwTerminate();
	}

	void Application::run()
	{
		/* show window */
		glfwShowWindow(m_window);
		
		/* Fps counter */
		RELEASE(
			float frameTimes = 0;
			float frames = 0;
			float updates = 0;
		)

		auto lastNow = NOW;
		RELEASE(auto last = NOW);

		float excess = 0;
		while (!glfwWindowShouldClose(m_window)) {
			auto now = NOW;
			const float dt = (now - lastNow).count() / 1e9f;
			lastNow = now;
			excess += dt;

			/* Update game logic*/
			while (m_timeStep <= excess) {
				excess -= m_timeStep;
				RELEASE(updates++);

				/* update here */
				update(m_timeStep);
			}

			RELEASE(auto renderStart = NOW);

			/* render */
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			render();
			glfwSwapBuffers(m_window);

			/* Update fps stats */
			RELEASE(
				frames++;
				now = NOW;
				frameTimes += (now - renderStart).count() / 1e9f;
			)

			/* Poll for and process events */
			glfwPollEvents();

			/* Log fps */
			RELEASE(
				float sinceLast = 1.f * (now - last).count();
				if (1e9 < sinceLast) {
					// update global stats
					sinceLast /= 1e9f;
					const float fps = frames / sinceLast;
					const float ups = updates / sinceLast;
					const float frameTimeMs = frameTimes / frames * 1000;
					APP_CORE_INFO("fps={.6f}, ups={.2f}, frame time={.6f}ms", fps, ups, frameTimeMs);

					frames = 0;
					updates = 0;
					frameTimes = 0;
					last = NOW;
				}
			)
		}

		/* Hide window */
		glfwHideWindow(m_window);
	}

	bool Application::process(ResizeEvent* e) {
		/* update size */
		m_windowWidth = float(e->width);
		m_windowHeight = float(e->height);

		/* update viewport */
		int width, height;
		glfwGetFramebufferSize(m_window, &width, &height);
		glViewport(0, 0, width, height);
		return false;
	}

	bool Application::process(KeyPressEvent* e) {
		if (e->key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(m_window, GL_TRUE);
		}
		return false;
	}
}
