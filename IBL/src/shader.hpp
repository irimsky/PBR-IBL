#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#include "utils.hpp"

class Shader
{
public:
    unsigned int ID;
    // constructor generates the shader on the fly
    // ------------------------------------------------------------------------
    Shader(){}
    Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        std::string vertexCode;
        std::string fragmentCode;
        std::string geometryCode;

     
        vertexCode = File::readText(vertexPath);
        fragmentCode = File::readText(fragmentPath);

        // 如果有几何着色
        if (geometryPath != nullptr)
        {
            geometryCode = File::readText(geometryPath);
        }
        
        const char* vShaderCode = vertexCode.c_str();
        const char* fShaderCode = fragmentCode.c_str();

        unsigned int vertex, fragment;

        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        std::cout << "Compiling Vertex Shader: " << vertexPath << std::endl;
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        std::cout << "Compiling Fragment Shader: " << fragmentPath << std::endl;
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        unsigned int geometry;
        if (geometryPath != nullptr)
        {
            const char* gShaderCode = geometryCode.c_str();
            geometry = glCreateShader(GL_GEOMETRY_SHADER);
            glShaderSource(geometry, 1, &gShaderCode, NULL);
            std::cout << "Compiling Geometry Shader: " << geometryPath << std::endl;
            glCompileShader(geometry);
            checkCompileErrors(geometry, "GEOMETRY");
        }

        // 链接着色器到程序
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        if (geometryPath != nullptr)
            glAttachShader(ID, geometry);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        glDetachShader(ID, vertex);
        glDeleteShader(vertex);
        glDetachShader(ID, fragment);
        glDeleteShader(fragment);
        if (geometryPath != nullptr) {
            glDetachShader(ID, geometry);
            glDeleteShader(geometry);
        }
    }

    void use()
    {
        glUseProgram(ID);
    }

    void deleteProgram()
    {
        glDeleteProgram(ID);
    }

    void setBool(const std::string& name, bool value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
    }

    void setInt(const std::string& name, int value) const
    {
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setFloat(const std::string& name, float value) const
    {
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
    }

    void setVec2(const std::string& name, const glm::vec2& value) const
    {
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec2(const std::string& name, float x, float y) const
    {
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
    }

    void setVec3(const std::string& name, const glm::vec3& value) const
    {
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec3(const std::string& name, float x, float y, float z) const
    {
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
    }

    void setVec4(const std::string& name, const glm::vec4& value) const
    {
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
    }
    void setVec4(const std::string& name, float x, float y, float z, float w)
    {
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w);
    }

    void setMat2(const std::string& name, const glm::mat2& mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat3(const std::string& name, const glm::mat3& mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }

    void setMat4(const std::string& name, const glm::mat4& mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }


protected:
    // 检查有无编译错误
    void checkCompileErrors(GLuint shader, std::string type)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                throw std::runtime_error(type + "Shader Compilation Error");
                std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                throw std::runtime_error("Program linking error");
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
            }
        }
    }
};

class ComputeShader : public Shader {
public:

    ComputeShader(){}
    ComputeShader(const char* computePath) 
    {
        std::string computeCode;
        computeCode = File::readText(computePath);
        const char* cShaderCode = computeCode.c_str();

        unsigned int compute;

        compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &cShaderCode, NULL);
        std::cout << "Compiling Compute Shader: " << computePath << std::endl;
        glCompileShader(compute);
        checkCompileErrors(compute, "COMPUTE");

        ID = glCreateProgram();
        glAttachShader(ID, compute);

        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        
        glDetachShader(ID, compute);
        glDeleteShader(compute);
    }

    void compute(GLuint x, GLuint y, GLuint z)
    {
        glDispatchCompute(x, y, z);
    }

};