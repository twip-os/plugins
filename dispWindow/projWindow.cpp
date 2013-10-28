#if linux
    #include "GL/glew.h"
    #include <unistd.h>
#else
    #include "GL/glew.h"
#endif
#include "projWindow.h"
#include <qevent.h>
#include <qstring.h>
#include <qstringlist.h>
#include <iostream>

#define _USE_MATH_DEFINES  // needs to be defined to enable standard declartions of PI constant
#include "math.h"

//----------------------------------------------------------------------------------------------------------------------------------
    #define BUFFER_OFFSET(bytes) ((GLubyte*) NULL + (bytes))

    const GLint POSITION = 0;
    GLsizei const ElementCount = 4;
	
	//! fragment and vertex shaders for gl v2 and gl v3
	//! the fragment shader multiplies input vertices with the transformation matrix MVP, the
	//! fragment shader calculates the texture pixel (and color) for each pixel. In addition a 
	//! gamma correction can be applied using a simple lookup vektor (lutarr)
	
    const char *VERTEX_SHADER_SOURCE = "  \
    #version 110                    \n\
                                    \
    uniform mat4 MVP;               \
    attribute vec4 position;        \
    varying vec2 TexCoord;          \
                                    \
    void main()                     \
    {                               \
        gl_Position = MVP * vec4(position.xy, 0.0, 1.0);	\
        TexCoord = position.zw;     \
    }                               \
    ";

    const char *VERTEX_SHADER_SOURCE130 = "  \
    #version 130                    \n\
                                    \
    uniform mat4 MVP;               \
    in vec4 position;               \
    out vec2 TexCoord;              \
                                    \
    void main()                     \
    {	                            \
        gl_Position = MVP * vec4(position.xy, 0.0, 1.0);	\
        TexCoord = position.zw;     \
    }                               \
    ";

    const char *FRAGMENT_SHADER_SOURCE = "    \
    #version 110                        \n\
                                        \
    uniform sampler2D texture;          \
    uniform int gamma;                  \
    uniform mat4 color;                 \
    varying vec2 TexCoord;              \
    uniform vec3 lutarr[256];           \
                                        \
    void main()                         \
    {                                   \
        if (gamma == 0)                 \
        {                               \
            gl_FragColor = color * texture2D(texture, TexCoord); \
        }                               \
        else                            \
        {                               \
            int col = int(texture2D(texture, TexCoord).r * 255.0);  \
            gl_FragColor = color * vec4(lutarr[col], 1.0);      \
        }                               \
    }                                   \
    ";

    const char *FRAGMENT_SHADER_SOURCE130 = " \
    #version 130                        \n\
                                        \
    uniform sampler2D texture;          \
    uniform int gamma;                  \
    uniform mat4 color;                 \
    in vec2 TexCoord;					\
    uniform vec3 lutarr[256];           \
    out vec4 FragColor;					\
                                        \
    void main()	                        \
    {	                                \
        if (gamma == 0)                 \
        {                               \
            FragColor = color * texture2D(texture, TexCoord); \
        }                               \
        else                            \
        {                               \
            int col = int(texture2D(texture, TexCoord).r * 255.0);  \
            FragColor = color * vec4(lutarr[col], 1.0);				\
        }                               \
    }                                   \
    ";

//----------------------------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------------------------
/** initialize openGL (below version two - i.e. using static pipelines)
*	@param [in]	width	window width
*	@param [in] height	window height
*	@return		zero for no error, openGL error code otherwise
*/
int initOGL2(const int width, const int height)
{
    int ret = 0;

    glMatrixMode(GL_PROJECTION);					//Projektionsmatrix w�hlen
    glLoadIdentity();
    gluOrtho2D(0, width, 0, height);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glEnable(GL_TEXTURE_2D);
    if ((ret = glGetError()))
    {
        std::cerr << "error enabeling texutres gl-window init\n";
        return ret;
    }

    GLfloat *par, *pag, *pab;

    par = (GLfloat*)calloc(256, sizeof(GLfloat));
    pag = (GLfloat*)calloc(256, sizeof(GLfloat));
    pab = (GLfloat*)calloc(256, sizeof(GLfloat));

    for (GLfloat i = 0; i < 256; i++)
    {
        par[(int)i] = i / 255.0;// * (cosGraycode.color & 1) / 1.0;
        pag[(int)i] = i / 255.0;// * (cosGraycode.color & 2) / 2.0;
        pab[(int)i] = i / 255.0;// * (cosGraycode.color & 4) / 4.0;
    }

//	glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &glval);

    glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, pag);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, par);
    glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, pab);

    free(par);
    free(pag);
    free(pab);

    glPixelTransferi(GL_RED_SCALE, 1);
    glPixelTransferi(GL_RED_BIAS, 0);
    glPixelTransferi(GL_GREEN_SCALE, 1);
    glPixelTransferi(GL_GREEN_BIAS, 0);
    glPixelTransferi(GL_BLUE_SCALE, 1);
    glPixelTransferi(GL_BLUE_BIAS, 0);
    glPixelTransferf(GL_ALPHA_SCALE, 0.0);
    glPixelTransferf(GL_ALPHA_BIAS,  1.0);

    glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

    if ((ret = glGetError()))
    {
        std::cerr << "error enabeling texutres gl-window init\n";
    }

    return ret;
}

//----------------------------------------------------------------------------------------------------------------------------------
/** initialize openGL (above version two - i.e. using vertex and fragment shaders)
*	@param [in]		glVer		openGL version
*	@param [out] 	ProgramName		reference to shader program on the gpu
*	@param [out]	UniformMVP		reference to transformation matrix on the gpu
*	@param [out] 	UniformLut		reference to lookup table memory on the gpu
*	@param [out] 	UniformGamma 	reference to gamma flag (gpu)
*	@param [out]	UniformTexture 	reference to texture buffer (gpu)
*	@param [out]	ArrayBufferName reference to array buffer (gpu)
*	@param [out]	ElementBufferName reference to element buffer (array buffer alignment) (gpu) 
*	@return		zero for no error, openGL error code otherwise
*
*	the function tries to compile the vertex and fragment shader code and to link the shader program. Afterwards
*	the variable positions for the parameters needed by the frag and vert shader are determined and returned.
*	The lut and the transformation are preloaded with standard values (linear lut and unity matrix).
*/
int initOGL3(const int glVer, GLuint &ProgramName, GLint &UniformMVP, GLint &UniformLut, GLint &UniformGamma,
        GLint &UniformTexture, GLint &UniformColor, GLuint &ArrayBufferName, GLuint &ElementBufferName)
{
    int ret = 0;
    char buf[1024];
    int len = 0;

    //!> create fragment and vertex shader
    GLuint VertexShader   = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    //!> load source code for fragment and vertex shader and change version number of vertex and
    //!> fragment shader code to match the set version of the opengl context, in order to avoid
    //!> backward compatible code generation (slow)
    char *VertFinal = NULL;
    char *FragFinal = NULL;
    ret = glGetError();
    if (glVer < 4096)
    {
        VertFinal = _strdup(VERTEX_SHADER_SOURCE);
        FragFinal = _strdup(FRAGMENT_SHADER_SOURCE);
    }
    else
    {
        VertFinal = _strdup(VERTEX_SHADER_SOURCE130);
        FragFinal = _strdup(FRAGMENT_SHADER_SOURCE130);
    }

    char *vertVerPos = strstr(VertFinal, "#version");
    char *fragVerPos = strstr(FragFinal, "#version");

    if (glVer >= 32768)
    {
        vertVerPos[9] = '1';
        vertVerPos[10] = '3';
        vertVerPos[11] = '0';

        fragVerPos[9] = '1';
        fragVerPos[10] = '3';
        fragVerPos[11] = '0';
    }
    else if (glVer >= 16384)
    {
        vertVerPos[9] = '1';
        vertVerPos[10] = '5';
        vertVerPos[11] = '0';

        fragVerPos[9] = '1';
        fragVerPos[10] = '5';
        fragVerPos[11] = '0';
    }
    else if (glVer >= 8192)
    {
        vertVerPos[9] = '1';
        vertVerPos[10] = '4';
        vertVerPos[11] = '0';

        fragVerPos[9] = '1';
        fragVerPos[10] = '4';
        fragVerPos[11] = '0';
    }
    else if (glVer >= 4096)
    {
        vertVerPos[9] = '1';
        vertVerPos[10] = '3';
        vertVerPos[11] = '0';

        fragVerPos[9] = '1';
        fragVerPos[10] = '3';
        fragVerPos[11] = '0';
    }
    else if (glVer >= 64)
    {
        vertVerPos[9] = '1';
        vertVerPos[10] = '2';
        vertVerPos[11] = '0';

        fragVerPos[9] = '1';
        fragVerPos[10] = '2';
        fragVerPos[11] = '0';
    }
    else if (glVer >= 32)
    {
        vertVerPos[9] = '1';
        vertVerPos[10] = '1';
        vertVerPos[11] = '0';

        fragVerPos[9] = '1';
        fragVerPos[10] = '1';
        fragVerPos[11] = '0';
    }

    glShaderSource(VertexShader, 1, (const GLchar**)&VertFinal, NULL);
    glShaderSource(FragmentShader, 1, (const GLchar**)&FragFinal, NULL);
    free(VertFinal);
    free(FragFinal);

    //!> compile vertex shader
    glCompileShader(VertexShader);
    glGetShaderiv(VertexShader, GL_COMPILE_STATUS, &ret);
    if (ret != GL_TRUE)
    {
        memset(buf, 0, 1024);
        glGetShaderInfoLog(VertexShader, 1024, &len, buf);
        std::cerr << "error compiling vertex shader\n" << buf << "\n";
    }

    //!> compile fragment shader
    glCompileShader(FragmentShader);
    glGetShaderiv(FragmentShader, GL_COMPILE_STATUS, &ret);
    if (ret != GL_TRUE)
    {
        memset(buf, 0, 1024);
        glGetShaderInfoLog(FragmentShader, 1024, &len, buf);
        std::cerr << "error compiling fragment shader\n" << buf << "\n";
    }

    //!> create program and attach compiled vertex and fragment shader to it
    ProgramName = glCreateProgram();
    glAttachShader(ProgramName, VertexShader);
    glAttachShader(ProgramName, FragmentShader);

    //!> link shader program
    glLinkProgram(ProgramName);
    glGetProgramiv(ProgramName, GL_LINK_STATUS, &ret);
    if (ret != GL_TRUE)
    {
        memset(buf, 0, 1024);
        glGetProgramInfoLog(ProgramName, 1024, &len, buf);
        std::cerr << "error linking shader program\n" << buf << "\n";
    }

    //!> retrieve location of uniform variables MVP and Diffuse from shader program
    UniformMVP = glGetUniformLocation(ProgramName, "MVP");
    UniformLut = glGetUniformLocation(ProgramName, "lutarr");
    UniformGamma = glGetUniformLocation(ProgramName, "gamma");
    UniformTexture = glGetUniformLocation(ProgramName, "texture");
    UniformColor = glGetUniformLocation(ProgramName, "color");

    // Compute the MVP (Model View Projection matrix)
    float MVP[4][4] = {
        {1.0, 0, 0, 0},
        {0, 1.0, 0, 0},
        {0, 0, 1.0, 0},
        {0, 0, 0, 1.0}
    };

    //!> Bind the program for use
    glUseProgram(ProgramName);

    //!> Set the value of coordinate transform (MVP) uniform.
    glUniformMatrix4fv(UniformMVP, 1, GL_FALSE, &MVP[0][0]);

    //!> Set the value of color calculation (initially white)
    glUniformMatrix4fv(UniformColor, 1, GL_FALSE, &MVP[0][0]);

    glUniform1i(UniformGamma, 0);

    GLint ElementSize = ElementCount * sizeof(GLint);
    GLint ElementData[ElementCount] = {0, 1, 2, 3};

    GLsizei const VertexCount = 4;
    GLsizeiptr PositionSize = VertexCount * 4 * sizeof(GLfloat);
    GLfloat PositionData[VertexCount*2][2] = {
        {-1.0f,-1.0f},	//Vertex
        {0.0f, 1.0},	//Texture

        {1.0f,-1.0f},	//V
        {1.0f, 1.0f},	//T

        {1.0f, 1.0f},	//V
        {1.0f, 0.0f},	//T

        {-1.0f, 1.0f},	//V
        {0.0f, 0.0f}	//T
    };

    //!> create vertex buffer on device
    glGenBuffers(1, &ArrayBufferName);
    glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferName);

    //!> copy vertex coordinates
    glBufferData(GL_ARRAY_BUFFER, PositionSize, PositionData, GL_STATIC_DRAW);
    //!> unbind buffer
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //!> generate use list on device
    glGenBuffers(1, &ElementBufferName);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferName);

    //!> copy use list to device
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, ElementSize, ElementData, GL_STATIC_DRAW);
    //!> unbind buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    //!> setting up initial gamma lut with linear response for rgb
    GLfloat templut[256][3];
    for (GLint col = 0; col < 256; col++)
    {
        templut[col][0] = col / 255.0;
        templut[col][1] = col / 255.0;
        templut[col][2] = col / 255.0;
    }

    glUniform3fv(UniformLut, 256, &templut[0][0]);
    glUseProgram(0);
    ret = glGetError();

    return ret;
}

//----------------------------------------------------------------------------------------------------------------------------------
PrjWindow::PrjWindow(const QMap<QString, ito::Param> &params, const QGLFormat &format, QWidget *parent, const QGLWidget *shareWidget, Qt::WindowFlags f)
    : QGLWidget(format, parent, shareWidget, f),
    m_glVer(-1),
    m_isInit(unInit),
    m_color(0),
    m_grayBitsVert(0),
    m_grayBitsHoriz(0),
    m_phaShift(4),
    m_period(12),
    m_orientation(0),
    m_gamma(0),
    m_imgNum(0),
    m_direction(1),
    m_gammaCol(0),
    m_lutTex(0),
    m_cosImgsVert(0),
    m_cosImgsHoriz(0),
    m_grayImgsVert(0),
    m_grayImgsHoriz(0),
    ProgramName(0),
    ArrayBufferName(0),
    ElementBufferName(0),
    UniformMVP(0),
    UniformTexture(0),
    UniformLut(0),
    UniformGamma(0),
    UniformColor(0)
{
    m_glVer = QGLFormat::openGLVersionFlags();

    int ret = 0;

    m_period = params["period"].getVal<int>();
    m_orientation = params["orientation"].getVal<int>();
    m_gamma = params["gamma"].getVal<int>();
    m_phaShift = params["phaseshift"].getVal<int>();
    m_color = params["color"].getVal<int>();

    m_lut.resize(256);
    for (int i = 0; i < 256; i++)
    {
        m_lut[i] = i;
    }

    makeCurrent();

    // set basic parameters
    if (m_glVer < QGLFormat::OpenGL_Version_2_0 /*32*/)
    {
        ret = initOGL2(width(), height());
    }
    else
    {
        if ((ret = glewInit()) != GLEW_OK)
        {
            m_isInit |= initFail;
        }
        if (ret == 0)
        {
            ret = initOGL3(m_glVer, ProgramName, UniformMVP, UniformLut, UniformGamma, UniformTexture, UniformColor, ArrayBufferName, ElementBufferName);
        }
        else
        {
            std::cerr << "error loading glew library: " << ret << "\n";
            m_isInit |= initFail;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	//Screen und Tiefenpuffer leeren

    if ((ret = glGetError()))
    {
        std::cerr << "error setting up projection window: " << ret << "\n";
    }

    doneCurrent();
}

//----------------------------------------------------------------------------------------------------------------------------------
PrjWindow::~PrjWindow()
{
    hide();
    m_isInit |= ~paramsValid;
    Sleep(100);

    if (m_isInit & initFail)
        return;

    if (m_glVer >= QGLFormat::OpenGL_Version_2_0 /*32*/)
    {
        glDeleteBuffers(1, &ElementBufferName);
        glDeleteBuffers(1, &ArrayBufferName);
        glDeleteProgram(ProgramName);
    }
    else
    {
//        glActiveTexture(GL_TEXTURE1);
//        glBindTexture(GL_TEXTURE_1D, m_lutTex);
//        glDeleteTextures(1, &m_lutTex);
//        glActiveTexture(GL_TEXTURE0);
    }

    m_lut.clear();
}

//----------------------------------------------------------------------------------------------------------------------------------
void PrjWindow::paintEvent(QPaintEvent * /*pevent*/)
{
    paintGL();
}

//----------------------------------------------------------------------------------------------------------------------------------
void PrjWindow::paintGL()
{
    static int drawScene = 0;

    if ((drawScene == 1) || (m_isInit != idleState) || (m_isInit & initFail))
        return;
    drawScene = 1;
    makeCurrent();

    // Set the display viewport
//	glViewport(0, 0, width, height);
//    ret = glGetError();

    if (m_imgNum == -1)
    {
        GLfloat red = 0, green = 0, blue = 0;

        if (m_color == 0)
        {
            red = m_gammaCol / 255.0;
        }
        else if (m_color == 1)
        {
            green = m_gammaCol / 255.0;
        }
        else if (m_color == 2)
        {
            blue = m_gammaCol / 255.0;
        }
        else
        {
            red = green = blue = m_gammaCol / 255.0;
        }

        glClearColor(red, green, blue, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else if (m_glVer < QGLFormat::OpenGL_Version_2_0 /*32*/)
    {
        int width = this->width();
        int height = this->height();

        glBindTexture(GL_TEXTURE_2D, m_texture[m_imgNum]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelTransferi(GL_MAP_COLOR, GL_TRUE);

        glBegin(GL_QUADS);
            glTexCoord2f(0, 1); glVertex3i(0, 0, 0);
            glTexCoord2f(1, 1); glVertex3i(width, 0, 0);
            glTexCoord2f(1, 0); glVertex3i(width, height, 0);
            glTexCoord2f(0, 0); glVertex3i(0, height, 0);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, 0);
    }
    else
    {
//        glClearColor(0.0f, 0.0f, 1.0f, 0.0f);	//black background
//        glClear(GL_COLOR_BUFFER_BIT);	//clear screen buffer

        //!> Bind shader program
        glUseProgram(ProgramName);

        glActiveTexture(GL_TEXTURE0);

        glBindTexture(GL_TEXTURE_2D, m_texture[m_imgNum]);
        glUniform1i(UniformTexture, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        //!> bind vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, ArrayBufferName);
        glVertexAttribPointer(POSITION, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), 0);
        //!> unbind buffer
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        //!> bind use list
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBufferName);

        //!> enable the previously set up attribute
        glEnableVertexAttribArray(POSITION);

        //!> draw buffers
        glDrawElements(GL_QUADS, ElementCount, GL_UNSIGNED_INT, 0);

        //!> disable the previously set up attributes
        glDisableVertexAttribArray(POSITION);

        glBindTexture(GL_TEXTURE_2D, 0);

        //!> Unbind shader program
        glUseProgram(0);
    }

//draw:
    //!> flush buffers, wait for drawing to finish and jic swap the buffers (we do not have double buffering)
//    swapBuffers();

//    if (ret = glGetError())
//    {
//        std::cerr << "error while drawing openGl scene: " << ret << "\n";
//    }
    glFlush();
    glFinish();

    doneCurrent();
    drawScene = 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::cosineExit()
{
    ito::RetVal retval(ito::retOk);

    int ret, n;

    if (m_isInit & cosIsInit)
    {
        // unset init flag and wait until drawing should be finished
        m_isInit &= ~cosIsInit;
        Sleep(100);

        if (m_glVer < QGLFormat::OpenGL_Version_2_0 /*32*/)
        {
            glDisable(GL_TEXTURE_2D);
            if ((ret = glGetError()))
            {
                std::cerr << "error disable texture (cosine exit)\n";
                retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        if ((ret = glGetError()))
        {
            std::cerr << "error unbind texture (cosine exit)\n";
            retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        }

        glDeleteTextures(m_phaShift, m_texture);
        if ((ret = glGetError()))
        {
            std::cerr << "error delete texture (cosine exit)\n";
            retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        }

        glDeleteTextures(m_phaShift, &m_texture[m_phaShift + m_grayBitsVert + 2]);
        if ((ret = glGetError()))
        {
            std::cerr << "error delete texture 2 (cosine exit)\n";
            retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        }
    }

    m_isInit &= ~cosIsInit;


    if (m_cosImgsHoriz != NULL)
    {
        for (n = 0; n < m_phaShift; n++)
        {
            if (m_cosImgsVert[n] != NULL)
            {
                free(m_cosImgsHoriz[n]);
            }
        }
        free(m_cosImgsHoriz);
        m_cosImgsHoriz = NULL;
    }

    if (m_cosImgsVert != NULL)
    {
        for (n = 0; n < m_phaShift; n++)
        {
            if (m_cosImgsVert[n]!=NULL)
            {
                free(m_cosImgsVert[n]);
            }
        }
        free(m_cosImgsVert);
        m_cosImgsVert = NULL;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::graycodeExit()
{
    ito::RetVal retval(ito::retOk);
    int ret, n;

    if (m_isInit & grayIsInit)
    {
        // unset init flag and wait until drawing should be finished
        m_isInit &= ~grayIsInit;
        Sleep(100);

        if (m_glVer < QGLFormat::OpenGL_Version_2_0 /*32*/)
        {
            glDisable(GL_TEXTURE_2D);
            if ((ret = glGetError()))
            {
                std::cerr << "error disable texture (graycode exit)\n";
                retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            }
        }

        glBindTexture(GL_TEXTURE_2D, 0);
        if ((ret = glGetError()))
        {
            std::cerr << "error unbind texture (graycode exit)\n";
            retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        }

        glDeleteTextures(m_grayBitsVert + 2, &m_texture[m_phaShift]);
        if ((ret = glGetError()))
        {
            std::cerr << "error delete texture (graycode exit)\n";
            retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        }

        glDeleteTextures(m_grayBitsVert + 2, &m_texture[m_phaShift * 2 + m_grayBitsVert + 2]);
        if ((ret = glGetError()))
        {
            std::cerr << "error delete texture 2 (graycode exit)\n";
            retval += ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        }
    }

    m_isInit &= ~grayIsInit;
    if (m_grayImgsHoriz != NULL)
    {
        for (n = 0; n < m_grayBitsHoriz + 2; n++)
        {
            if (m_grayImgsHoriz[n] != NULL)
            {
                free(m_grayImgsHoriz[n]);
            }
        }
        free(m_grayImgsHoriz);
        m_grayImgsHoriz = NULL;
    }

    if (m_grayImgsVert != NULL)
    {
        for (n = 0; n < m_grayBitsVert + 2; n++)
        {
            if (m_grayImgsVert[n] != NULL)
            {
                free(m_grayImgsVert[n]);
            }
        }
        free(m_grayImgsVert);
        m_grayImgsVert = NULL;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::cosineInit()
{
    ito::RetVal retval(ito::retOk);

    long i = 0, j = 0, k = 0, m = 0;
    int ret = 0;
    int width = this->width();
    int height = this->height();
    unsigned char **phasedummy = NULL, *tempimg=NULL;

    double *phaseVals = NULL;
    double minval;
    double maxval;

    if (!(m_isInit & paramsValid) || (m_isInit & initFail))
    {
        return ito::retOk;
    }

    if (m_isInit & cosIsInit)
    {
        cosineExit();
    }

    if ((phasedummy = (unsigned char**)malloc(m_phaShift * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "error out of memory (cosine init 1)\n";
        retval = ito::retError;
        goto end;
    }
    if ((m_cosImgsVert = (unsigned char**)malloc(m_phaShift * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "error out of memory (cosine init 2)\n";
        retval = ito::retError;
        goto end;
    }
    if ((m_cosImgsHoriz = (unsigned char**)malloc(m_phaShift * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "error out of memory (cosine init 3)\n";
        retval = ito::retError;
        goto end;
    }

    for(i = 0; i < m_phaShift; i++)
    {
        if((phasedummy[i] = (unsigned char*)malloc(m_period * sizeof(*phasedummy[i]))) == NULL)
        {
            std::cerr << "error out of memory (cosine init 4)\n";
            retval = ito::retError;
            goto end;
        }
        if((m_cosImgsVert[i] = (unsigned char*)malloc(width * height)) == NULL)
        {
            std::cerr << "error out of memory (cosine init 5)\n";
            retval = ito::retError;
            goto end;
        }
        if((m_cosImgsHoriz[i] = (unsigned char*)malloc(width * height)) == NULL)
        {
            std::cerr << "error out of memory (cosine init 6)\n";
            retval = ito::retError;
            goto end;
        }
    }

    //!> in the following loop the lookuptable values are written to the
    //!> two dimensinal array phasedummy, according to
    //!> phasedummy[i] = cos(x - i * PI / 2)
    phaseVals = new double[m_period];
    minval = 10;
    maxval = -10;

    for (j = 0; j < m_period / 2; j++)
    {
        phaseVals[m_period - j - 1] = phaseVals[j] = (cos((double)M_PI * 2.0 * ((j + 0.5) / ((double)m_period))));
        if (phaseVals[j] < minval)
            minval = phaseVals[j];
        if (phaseVals[j] > maxval)
            maxval = phaseVals[j];
    }

    for (i = 0; i < m_phaShift; i++)
    {
        //!> Cosine fringes to right
        if (m_direction > 0)
        {
            for (j = 0; j < m_period; j++)
            {
                phasedummy[i][j] = (unsigned char)((phaseVals[(m_period - (j - (m_period / m_phaShift) * i)) % m_period] - minval) / (maxval - minval) * 255.0);
    //            phasedummy[i][j] = (BYTE)((phaseVals[j] + 1.0) / 2.0 * 255.0);
            }
        }
        //!> Cosine fringes to left
        else
        {
            for (j = 0; j < m_period; j++)
            {
                phasedummy[i][j] = (unsigned char)((phaseVals[(j + (m_period / m_phaShift) * i) % m_period] - minval) / (maxval - minval) * 255.0);
    //            phasedummy[i][j] = (BYTE)((phaseVals[j] + 1.0) / 2.0 * 255.0);
            }
        }
    }
    delete phaseVals;

    //!> filling of the "images"
    if ((tempimg = (unsigned char*)calloc(width * height, sizeof(unsigned char))) == 0)
    {
        std::cerr << "error out of memory (cosine init 7)\n";
        retval = ito::retError;
        goto end;
    }

    glGenTextures(m_phaShift, m_texture);
    if ((ret = glGetError()))
    {
        std::cerr << "error gen texture (cosine init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    glGenTextures(m_phaShift, &m_texture[m_phaShift + m_grayBitsVert + 2]);
    if ((ret = glGetError()))
    {
        std::cerr << "error gen texture (graycode / cosine init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    //!> we run this loop twice as first vertical and then horizontal fringe patterns are generated
    for (i = 0; i < 2 * m_phaShift; i++)
    {
        memset(tempimg, 0, width * height * sizeof(unsigned char));

        //!> vertical fringes
        if(i < m_phaShift)
        {
            for (m = 0; m < height; m++)
            {
                for (j = 0; j < width; j += m_period)
                {
                    for (k = 0; k < m_period; k++)
                    {
                        if((j + k) < width)
                        {
                            tempimg[m * width + j + k] = phasedummy[i][k];
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            memcpy(m_cosImgsVert[i], tempimg, height * width);
        }
        //!> horizontal fringes
        else
        {
            for (m = 0; m < height; m += m_period)
            {
                for (k = 0; k < m_period; k++)
                {
                    for (j = 0; j < width; j++)
                    {
                        if((m + k) < height)
                        {
                            tempimg[(m + k) * width + j] = phasedummy[i - m_phaShift][k];
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }
            memcpy(m_cosImgsHoriz[i - m_phaShift], tempimg, height * width);
        }

        if (i < m_phaShift)
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[i]);
        }
        else
        {
            glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[i + m_grayBitsVert + 2]);
        }

        if ((ret = glGetError()))
        {
            std::cerr << "error bind texture (cosine init)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
        if ((ret = glGetError()))
        {
            std::cerr << "error tex image (cosine init)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

end:
    glBindTexture(GL_TEXTURE_2D, 0);
    if (phasedummy != NULL)
    {
        for(i = 0; i < m_phaShift; i++)
        {
            if (phasedummy[i] != NULL)
                free(phasedummy[i]);
        }
        free(phasedummy);
    }
    if (tempimg)
    {
        free(tempimg);
    }

    if (retval == ito::retOk)
    {
        m_isInit |= cosIsInit;
    }
    else
    {
        m_isInit &= ~cosIsInit;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::graycodeInit()
{
    ito::RetVal retval(ito::retOk);

    long i, j, k;
    long loopendy, loopendx, widthVert, widthHoriz;
    int ret = 0;
    int width = this->width();
    int height = this->height();
    unsigned char **grayVert = NULL, **grayHoriz = NULL, *tempimg = NULL;
    
    if (!(m_isInit & paramsValid) || (m_isInit & initFail))
    {

        return retval;
    }

    if (m_isInit & grayIsInit)
    {
        graycodeExit();
    }

    widthVert = widthHoriz = m_period;
    for (i = 0; i < m_grayBitsVert - 1; i++)
    {
        widthVert *= 2;
    }
    for (i = 0; i < m_grayBitsHoriz - 1; i++)
    {
        widthHoriz *= 2;
    }

    if ((tempimg = (unsigned char *)calloc(width * height, sizeof(unsigned char))) == NULL)
    {
        std::cerr << "out of memory (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    //local graycode sequenz
    if((grayVert = (unsigned char**)malloc((m_grayBitsVert + 2) * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "out of memory (graycode init 1)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    if((grayHoriz = (unsigned char**)malloc((m_grayBitsHoriz + 2) * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "out of memory (graycode init 2)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    if((m_grayImgsVert = (unsigned char**)malloc((m_grayBitsVert + 2) * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "out of memory (graycode init 3)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    if((m_grayImgsHoriz = (unsigned char**)malloc((m_grayBitsHoriz + 2) * sizeof(unsigned char*))) == NULL)
    {
        std::cerr << "out of memory (graycode init 4)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    for(i = 0; i < m_grayBitsVert + 2; i++)
    {
        if((grayVert[i] = (unsigned char *)calloc(widthVert, sizeof(unsigned char))) == NULL)
        {
            std::cerr << "out of memory (graycode init 5)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
    }
    for(i = 0; i < m_grayBitsHoriz + 2; i++)
    {
        if((grayHoriz[i] = (unsigned char *)calloc(widthHoriz, sizeof(unsigned char))) == NULL)
        {
            std::cerr << "out of memory (graycode init 6)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
    }

    for(i = 0; i < m_grayBitsVert + 2; i++)
    {
        if((m_grayImgsVert[i] = (unsigned char *)malloc(width * height)) == NULL)
        {
            std::cerr << "out of memory (graycode init 7)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
    }
    for(i = 0; i < m_grayBitsHoriz + 2; i++)
    {
        if((m_grayImgsHoriz[i] = (unsigned char *)malloc(width * height)) == NULL)
        {
            std::cerr << "out of memory (graycode init 8)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
    }

    //create first vertical image
    for (i = 0; i < widthVert; i++)
    {
        if (i < widthVert / 2)
        {
            grayVert[0][i] = 0;
        }
        else
        {
            grayVert[0][i] = 255;
        }
    }
    for (i = 0; i < widthHoriz; i++)
    {
        if (i < widthHoriz / 2)
        {
            grayHoriz[0][i] = 0;
        }
        else
        {
            grayHoriz[0][i] = 255;
        }
    }

    //other images are filled
    for (j = 1; j < m_grayBitsVert; j++)
    {
        for (i = 0; i < widthVert; i++)
        {
            if(i < widthVert / 2)
            {
                grayVert[j][i] = grayVert[j - 1][2 * i];
            }
            else
            {
                grayVert[j][i] = grayVert[j][widthVert - i - 1];
            }
        }
    }
    for (j = 1; j < m_grayBitsHoriz; j++)
    {
        for (i = 0; i < widthHoriz; i++)
        {
            if(i < widthHoriz / 2)
            {
                grayHoriz[j][i] = grayHoriz[j - 1][2 * i];
            }
            else
            {
                grayHoriz[j][i] = grayHoriz[j][widthHoriz - i - 1];
            }
        }
    }

    if(width < widthVert)
    {
        loopendx = width;
    }
    else
    {
        loopendx = widthVert;
    }

    glGenTextures(m_grayBitsVert + 2, &m_texture[m_phaShift]);
    if ((ret = glGetError()))
    {
        std::cerr << "error genTextures 2 (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    glGenTextures(m_grayBitsHoriz + 2, &m_texture[m_phaShift * 2 + m_grayBitsVert + 2]);
    if ((ret = glGetError()))
    {
        std::cerr << "error genTextures 2 (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    memset(tempimg, 0, width * height * sizeof(unsigned char));
    memcpy(m_grayImgsVert[0], tempimg, width * height);
    memcpy(m_grayImgsHoriz[0], tempimg, width * height);

    glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[m_phaShift]);
    if ((ret = glGetError()))
    {
        std::cerr << "error bind texture black image (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
    if ((ret = glGetError()))
    {
        std::cerr << "error tex image black image (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[2 * m_phaShift + m_grayBitsVert + 2]);
    if ((ret = glGetError()))
    {
        std::cerr << "error bind texture black image 2 (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
    if ((ret = glGetError()))
    {
        std::cerr << "error tex image black image 2 (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    memset(tempimg, 255, width * height * sizeof(unsigned char));
    memcpy(m_grayImgsVert[1], tempimg, width * height);
    memcpy(m_grayImgsHoriz[1], tempimg, width * height);

    glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[m_phaShift + 1]);
    if ((ret = glGetError()))
    {
        std::cerr << "error bind texture white image (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
    if ((ret = glGetError()))
    {
        std::cerr << "error tex image white image (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[2 * m_phaShift + m_grayBitsVert + 3]);
    if ((ret = glGetError()))
    {
        std::cerr << "error bind texture white image 2 (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
    if ((ret = glGetError()))
    {
        std::cerr << "error tex image white image 2 (graycode init)\n";
        retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
        goto end;
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    for(i = 0; i < m_grayBitsVert; i++)
    {
        memset(tempimg, 0, width * height * sizeof(unsigned char));
        for(j = 0; j < loopendx;j ++)
        {
            if(grayVert[i][j] == 255)
            {
                for (k = 0; k < height; k++)
                {
                    tempimg[k * width + j] |= 255;
                }
            }
        }
        memcpy(m_grayImgsVert[i + 2], tempimg, width * height);

        glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[m_phaShift + i + 2]);
        if ((ret = glGetError()))
        {
            std::cerr << "error bind texture (graycode init)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
        if ((ret = glGetError()))
        {
            std::cerr << "error tex image (graycode init)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    //!> bitwise copying
    if(height < widthHoriz)
    {
        loopendy = height;
    }
    else
    {
        loopendy = widthHoriz;
    }

    for(i = 0; i < m_grayBitsHoriz; i++)
    {
        memset(tempimg, 0, width * height * sizeof(unsigned char));
        for(j = 0; j < loopendy; j++)
        {
            if(grayHoriz[i][j] == 255)
            {
                for(k = 0; k < width; k++)
                {
                    tempimg[j * width + k] |= 255;
                }
            }
        }
        memcpy(m_grayImgsHoriz[i + 2], tempimg, width * height);

        glBindTexture(GL_TEXTURE_2D, (GLuint)m_texture[m_phaShift * 2 + m_grayBitsVert + i + 4]);
        if ((ret = glGetError()))
        {
            std::cerr << "error bind texture (graycode init)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, tempimg);
        if ((ret = glGetError()))
        {
            std::cerr << "error tex image (graycode init)\n";
            retval = ito::RetVal(ito::retError, ret, tr("").toAscii().data());
            goto end;
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

end:
    glBindTexture(GL_TEXTURE_2D, 0);
    if (grayVert)
    {
        for(i = 0; i < m_grayBitsVert + 2; i++)
        {
            if (grayVert[i])
            {
                free(grayVert[i]);
            }
        }
        free(grayVert);
        grayVert = NULL;
    }

    if (grayHoriz)
    {
        for(i = 0; i < m_grayBitsHoriz + 2; i++)
        {
            if (grayHoriz[i])
            {
                free(grayHoriz[i]);
            }
        }
        free(grayHoriz);
        grayHoriz = NULL;
    }

    if (tempimg)
    {
        free(tempimg);
    }

    if (retval == ito::retOk)
    {
        m_isInit |= grayIsInit;
    }
    else
    {
        m_isInit &= ~grayIsInit;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::setupProjection()
{
    ito::RetVal retval(ito::retOk);

    div_t periodTest, test;
    unsigned char bitsTemp;

    // test if smallest fringe has a width (in pixels) that is a multiple of 2 (pixels)
    test = div((int)m_period, (int)2);
    if (test.rem != 0)
    {
        m_period++;
        std::cerr << "for a combination of graycode and cosine fringes the period of the \
                      cosine fringes must be a mutliple of 2. The cosine fringe period was \
                      changed to " << m_period << " pixels!\n";
    }

    // period must dividable by the number of shifts
    periodTest = div((int)m_period, (int)m_phaShift);
    if(periodTest.rem != 0)
    {
        m_period = ceil((float)m_period / (float)m_phaShift) * (float)m_phaShift;
        std::cerr << "the period number of the cosine fringes must be dividable by the \
                      number of phaseshifts. Therefore the period number was changed to" << m_period << " pixels!\n";
    }

    if (m_period < width())
    {
        bitsTemp = floor(log(width() / (float)m_period) / log(2.0));
    }
    else
    {
        bitsTemp = 0;
    }
    if (pow(2.0, (double)bitsTemp) < width() / (float)m_period)
    {
        bitsTemp++;
    }
    m_grayBitsVert = bitsTemp + 1;
    // one graycode bit is minimum
    if (m_grayBitsVert < 1)
    {
        m_grayBitsVert++;
    }

    if (m_period < height())
    {
        bitsTemp = floor(log(height() / (float)m_period) / log(2.0));
    }
    else
    {
        bitsTemp = 0;
    }
    if (pow(2.0, (double)bitsTemp) < height() / (float)m_period)
    {
        bitsTemp++;
    }
    m_grayBitsHoriz = bitsTemp + 1;
    // one graycode bit is minimum
    if (m_grayBitsHoriz < 1)
    {
        m_grayBitsHoriz++;
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
void PrjWindow::setSize(int sizex, int sizey, bool reCalcGL)
{
    m_isInit &= ~paramsValid;
    Sleep(100);

    resize(sizex, sizey);

    makeCurrent();
    cosineExit();
    graycodeExit();

    setupProjection();

    int width = this->width();
    int height = this->height();
    // Set the display viewport
    glViewport(0, 0, width, height);

    m_isInit |= paramsValid;

    if(reCalcGL)
    {
        cosineInit();
        graycodeInit();
        numberOfImagesChanged(this->getNumImages(), this->getNumGrayImages(), this->getPhaseShift());
    }
    doneCurrent();
}

//----------------------------------------------------------------------------------------------------------------------------------
void PrjWindow::setPos(int xpos, int ypos)
{
    move(xpos, ypos);
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::calcLUT(QVector<double> *grayvalues, QVector<unsigned char> *lut)
{
    ito::RetVal retval = ito::retOk;

    if (grayvalues->size() < 64)
    {
        retval = ito::RetVal(ito::retError, 0, tr("insufficient gray values").toAscii().data());
    }

    double minval = 10000;
    double maxval = -1;

    //calc min and max grayvalue, which has been given to this method as parameter
    for (int gval = 0; gval < grayvalues->size(); gval++)
    {
        if (grayvalues->at(gval) < minval)
            minval = grayvalues->at(gval);
        if (grayvalues->at(gval) > maxval)
            maxval = grayvalues->at(gval);
    }

    //normGVals is vector where the contrast of each gray value is stored (normalized gray value: 0<=value<=1)
    QVector<double> normGVals;
    normGVals.resize(grayvalues->size());
    for (int gval = 0; gval < grayvalues->size(); gval++)
    {
        normGVals[gval] = (grayvalues->at(gval) - minval) / (maxval - minval);
    }

    lut->resize(256);
    double fak = 255.0 / (double)normGVals.size();
    for (int gval = 0; gval < 256; gval++)
    {
        double val = gval / 255.0;
        for (int n = 1; n < normGVals.size(); n++)
        {
            if ((normGVals[n - 1] <= val) && (normGVals[n] >= val))
            {
                (*lut)[gval] = floor((n + (val - normGVals[n - 1]) / (normGVals[n] - normGVals[n - 1])) * fak + 0.5);
            }
        }
    }
    normGVals.clear();
    (*lut)[0] = 0;
    (*lut)[255] = 255;

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::setLUT(QVector<unsigned char> *lut)
{
    m_lut.clear();
    m_lut = *lut;
    ito::RetVal retval = ito::retOk;

    //!> setting up initial gamma lut with linear response for rgb
    GLfloat templut[256][3];
    for (int col = 0; col < 256; col++)
    {
        templut[col][0] = m_lut[col] / 255.0;
        templut[col][1] = m_lut[col] / 255.0;
        templut[col][2] = m_lut[col] / 255.0;
    }

    int oldval = m_isInit;
    m_isInit &= ~paramsValid;

    makeCurrent();
    //!> Bind the program for use
    glUseProgram(ProgramName);
    glUniform3fv(UniformLut, 256, &templut[0][0]);
    glUseProgram(0);
    doneCurrent();
    m_isInit |= oldval;

    paintGL();

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::setColor(const int col)
{
    ito::RetVal retval = ito::retOk;
    int ret = 0;

    if (m_glVer <= QGLFormat::OpenGL_Version_2_0 /*32*/)
    {
        GLint glval;
        GLfloat *par, *pag, *pab;
        int colbit;

        if (col == 0)
            colbit = 1;
        else if (col == 1)
            colbit = 2;
        else if (col == 2)
            colbit = 4;
        else
            colbit = 7;

        par = (GLfloat*)calloc(256, sizeof(GLfloat));
        pag = (GLfloat*)calloc(256, sizeof(GLfloat));
        pab = (GLfloat*)calloc(256, sizeof(GLfloat));

        for (float i = 0; i < 256; i++)
        {
            par[(int)i] = i / 255.0 * (colbit & 1);
            pag[(int)i] = i / 255.0 * (colbit & 2) / 2.0;
            pab[(int)i] = i / 255.0 * (colbit & 4) / 4.0;
        }

        makeCurrent();
        glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &glval);
        ret = glGetError();

        glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, pag);
        ret = glGetError();
        glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, par);
        ret = glGetError();
        glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, pab);
        ret = glGetError();

        free(par);
        free(pag);
        free(pab);

        glPixelTransferi(GL_RED_SCALE, 1);
        glPixelTransferi(GL_RED_BIAS, 0);
        glPixelTransferi(GL_GREEN_SCALE, 1);
        glPixelTransferi(GL_GREEN_BIAS, 0);
        glPixelTransferi(GL_BLUE_SCALE, 1);
        glPixelTransferi(GL_BLUE_BIAS, 0);
        glPixelTransferf(GL_ALPHA_SCALE, 0.0);
        glPixelTransferf(GL_ALPHA_BIAS,  1.0);

        glPixelTransferi(GL_MAP_COLOR, GL_TRUE);
        doneCurrent();

        paintGL();
    }
    else
    {
        float color[4][4] = {
            {1.0, 0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0, 0.0},
            {0.0, 0.0, 0.0, 1.0},
        };

        switch (col)
        {
            case 0:
                color[1][1] = 0;
                color[2][2] = 0;
            break;

            case 1:
                color[0][0] = 0;
                color[2][2] = 0;
            break;

            case 2:
                color[0][0] = 0;
                color[1][1] = 0;
            break;
        }

        int oldval = m_isInit;
        m_isInit &= ~paramsValid;
        makeCurrent();
        //!> Bind the program for use
        glUseProgram(ProgramName);
        //!> Set the value of color calculation (initially white)
        glUniformMatrix4fv(UniformColor, 1, GL_FALSE, &color[0][0]);
        //!> Bind the program for use
        glUseProgram(0);
        doneCurrent();
        m_isInit |= oldval;

        paintGL();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::setOrientation(const int orient)
{
    ito::RetVal retval = ito::retOk;

    if (orient <= 0)
    {
        m_orientation = 0;
        if (m_imgNum < 0)
        {
            m_imgNum = 0;
        }
        else if (m_imgNum >= m_phaShift + m_grayBitsVert + 2)
        {
            m_imgNum = m_phaShift + m_grayBitsVert + 1;
        }
    }
    else
    {
        m_orientation = 1;
        if (m_imgNum < m_phaShift + m_grayBitsVert + 2)
        {
            m_imgNum = m_phaShift + m_grayBitsVert + 2;
        }
        else if (m_imgNum >=  m_phaShift + m_grayBitsVert + 2 + m_phaShift + m_grayBitsHoriz + 2)
        {
            m_imgNum =  m_phaShift + m_grayBitsVert + 2 + m_phaShift + m_grayBitsHoriz + 1;
        }
    }

    paintGL();

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
/*ito::RetVal PrjWindow::setPhaseShift(const int phaseshift)
{
    ito::RetVal retval = ito::retOk;
    int oldval = m_isInit;
    m_isInit &= ~paramsValid;
    Sleep(100);

    makeCurrent();
    retval += cosineExit();
    retval += graycodeExit();

    m_phaShift = phaseshift;
    retval += setupProjection();

    retval += cosineInit();
    retval += graycodeInit();
    doneCurrent();
    m_isInit |= oldval;

    paintGL();

    return retval;
}*/

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::setGamma(const int gamma)
{
    ito::RetVal retval = ito::retOk;

    if (gamma)
    {
        m_gamma = 1;
    }
    else
    {
        m_gamma = 0;
    }

    if (m_glVer >= QGLFormat::OpenGL_Version_2_0 /*32*/)
    {
        //!> setting up initial gamma lut with linear response for rgb
        GLfloat templut[257][3];
        for (int col = 0; col < 256; col++)
        {
            templut[col][0] = m_lut[col] / 255.0;
            templut[col][1] = m_lut[col] / 255.0;
            templut[col][2] = m_lut[col] / 255.0;
        }

        int oldval = m_isInit;
        m_isInit &= ~paramsValid;
        makeCurrent();
        //!> Bind the program for use
        glUseProgram(ProgramName);
        glUniform3fv(UniformLut, 256, &templut[0][0]);
        //!> Set the value of color calculation (initially white)
        glUniform1i(UniformGamma, m_gamma);
        //int ret = glGetError();
        //!> Bind the program for use
        glUseProgram(0);
        doneCurrent();
        m_isInit |= oldval;
    }
    else
    {
        GLfloat *par, *pag, *pab;

        par = (GLfloat*)calloc(256, sizeof(GLfloat));
        pag = (GLfloat*)calloc(256, sizeof(GLfloat));
        pab = (GLfloat*)calloc(256, sizeof(GLfloat));

        for (float i = 0; i < 256; i++)
        {
            par[(int)i] = m_lut[i] / 255.0;
            pag[(int)i] = m_lut[i] / 255.0;
            pab[(int)i] = m_lut[i] / 255.0;
        }

    //	glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE, &glval);

        glPixelMapfv(GL_PIXEL_MAP_I_TO_G, 256, pag);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_R, 256, par);
        glPixelMapfv(GL_PIXEL_MAP_I_TO_B, 256, pab);

        free(par);
        free(pag);
        free(pab);
    }

    paintGL();

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::setGammaPrj(const int gammaCol)
{
    ito::RetVal retval = ito::retOk;

    m_gammaCol = gammaCol;
    m_imgNum = -1;

    paintGL();

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::showFirstImg(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);

    m_imgNum = 0;

    paintGL();

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::showNextImg(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);

    if (m_orientation <= 0)
    {
        if (m_imgNum < m_phaShift + m_grayBitsVert + 1)
        {
            m_imgNum++;
        }
    }
    else
    {
        if (m_imgNum < m_phaShift + m_grayBitsHoriz + 1)
        {
            m_imgNum++;
        }
    }

    paintGL();

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::showFirstGrayImg(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);
    m_imgNum = 0;

    paintGL();

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::showFirstCosImg(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);

    if (m_orientation <= 0)
    {
        m_imgNum = 2 + m_grayBitsVert;
    }
    else
    {
        m_imgNum = 2 + m_grayBitsHoriz;
    }

    paintGL();

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }

    return retval;
}

//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::showImageNum(const int num)
{
    if (m_orientation <= 0)
    {
        if (num < 0)
        {
            m_imgNum = 0;
        }
        else if (num < m_phaShift + m_grayBitsVert + 2)
        {
            m_imgNum = num;
        }
        else
        {
            m_imgNum = m_phaShift + m_grayBitsVert + 1;
        }
    }
    else
    {
        if (num < 0)
        {
            m_imgNum = num + m_phaShift + m_grayBitsVert + 2;
        }
        else if (num < m_phaShift + m_grayBitsVert + 2 + m_phaShift + m_grayBitsHoriz + 2)
        {
            m_imgNum = num + m_phaShift + m_grayBitsVert + 2;
        }
        else
        {
            m_imgNum = m_phaShift + m_grayBitsVert + 2 + m_phaShift + m_grayBitsHoriz + 1;
        }
    }

    paintGL();

    return ito::retOk;
}

//----------------------------------------------------------------------------------------------------------------------------------
int PrjWindow::getOrientationClearedCurImg(void)
{
	if (m_orientation <= 0)
    {
		return m_imgNum;
        
    }
    else
    {
		return ( m_imgNum - (m_phaShift + m_grayBitsVert + 2) );
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
void PrjWindow::resizeEvent(QResizeEvent *pevent)
{
    QSize newSize = pevent->size();
    resize(newSize.width(), newSize.height());
}

//----------------------------------------------------------------------------------------------------------------------------------
int PrjWindow::getNumImages()
{
    if (m_orientation <= 0)
    {
        return m_phaShift + m_grayBitsVert + 2;
    }
    else
    {
        return m_phaShift + m_grayBitsHoriz + 2;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
int PrjWindow::getNumGrayImages(void)
{
    if (m_orientation <= 0)
    {
        return m_grayBitsVert;
    }
    else
    {
        return m_grayBitsHoriz;
    }
}
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::shotDown(ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);
    hide();
    disableInit();
    Sleep(100);

    makeCurrent();
    cosineExit();
    graycodeExit();
    doneCurrent();

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }
    return retval;
}
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::configProjection(int period, int phaseShift, int orient, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);
    
    int oldval = m_isInit;

    m_isInit &= ~paramsValid;
    Sleep(100);

    makeCurrent();
//    retval += cosineExit();
//    retval += graycodeExit();

    m_phaShift = phaseShift;
    m_period = period;
    retval += setupProjection();

    m_isInit |= paramsValid;

    retval += cosineInit();
    retval += graycodeInit();

    doneCurrent();

    if(!retval.containsError())
    {
        m_isInit |= paramsValid;

        if (orient <= 0)
        {
            m_orientation = 0;
            if (m_imgNum < 0)
            {
                m_imgNum = 0;
            }
            else if (m_imgNum >= m_phaShift + m_grayBitsVert + 2)
            {
                m_imgNum = m_phaShift + m_grayBitsVert + 1;
            }
        }
        else
        {
            m_orientation = 1;
            if (m_imgNum < m_phaShift + m_grayBitsVert + 2)
            {
                m_imgNum = m_phaShift + m_grayBitsVert + 2;
            }
            else if (m_imgNum >=  m_phaShift + m_grayBitsVert + 2 + m_phaShift + m_grayBitsHoriz + 2)
            {
                m_imgNum =  m_phaShift + m_grayBitsVert + 2 + m_phaShift + m_grayBitsHoriz + 1;
            }
        }
    }
    paintGL();

    numberOfImagesChanged(this->getNumImages(), this->getNumGrayImages(), this->getPhaseShift());

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }
    return retval;
}
//----------------------------------------------------------------------------------------------------------------------------------
ito::RetVal PrjWindow::configProjectionFull(int xpos, int sizex, int ypos, int sizey, int period, int phaseShift, int orient, ItomSharedSemaphore *waitCond)
{
    ItomSharedSemaphoreLocker locker(waitCond);
    ito::RetVal retval(ito::retOk);

    setPos(xpos, ypos);
    setSize(sizex, sizey, false);

    retval += configProjection(period, phaseShift, orient);

    if (waitCond)
    {
        waitCond->returnValue = retval;
        waitCond->release();
    }
    return retval;
}
