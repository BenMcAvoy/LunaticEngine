#pragma once

#include "pch.h"

GLFWwindow* createWindow(int width, int height, const char* title);
void renderWith(std::function<void(GLFWwindow*)> renderFunc);
