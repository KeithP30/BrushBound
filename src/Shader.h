#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

class Shader {
public:
    unsigned int ID;

    Shader(const char* vertPath, const char* fragPath) {
        std::string vCode, fCode;
        std::ifstream vFile, fFile;
        vFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        fFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
        try {
            vFile.open(vertPath); fFile.open(fragPath);
            std::stringstream vs, fs;
            vs << vFile.rdbuf(); fs << fFile.rdbuf();
            vFile.close(); fFile.close();
            vCode = vs.str(); fCode = fs.str();
        } catch (...) {
            std::cerr << "ERROR: Shader file not found\n";
        }
        const char* vSrc = vCode.c_str();
        const char* fSrc = fCode.c_str();

        unsigned int vert = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vert, 1, &vSrc, NULL);
        glCompileShader(vert);
        checkErrors(vert, "VERTEX");

        unsigned int frag = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(frag, 1, &fSrc, NULL);
        glCompileShader(frag);
        checkErrors(frag, "FRAGMENT");

        ID = glCreateProgram();
        glAttachShader(ID, vert);
        glAttachShader(ID, frag);
        glLinkProgram(ID);
        checkErrors(ID, "PROGRAM");

        glDeleteShader(vert);
        glDeleteShader(frag);
    }

    void use() { glUseProgram(ID); }

    void setMat4(const std::string& name, const glm::mat4& m) const {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(m));
    }
    void setVec3(const std::string& name, const glm::vec3& v) const {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, glm::value_ptr(v));
    }
    void setFloat(const std::string& name, float v) const {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), v);
    }

private:
    void checkErrors(unsigned int shader, const std::string& type) {
        int success; char log[1024];
        if (type != "PROGRAM") {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success) {
                glGetShaderInfoLog(shader, 1024, NULL, log);
                std::cerr << "SHADER ERROR [" << type << "]: " << log << "\n";
            }
        } else {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success) {
                glGetProgramInfoLog(shader, 1024, NULL, log);
                std::cerr << "PROGRAM ERROR: " << log << "\n";
            }
        }
    }
};
