#ifndef PROJWINDOW_H
#define PROJWINDOW_H

#include "common/sharedStructures.h"
#include "common/sharedStructuresQt.h"

#include <QtOpenGL/qgl.h>

//----------------------------------------------------------------------------------------------------------------------------------
class PrjWindow : public QGLWidget
{
    Q_OBJECT

    public:
        PrjWindow(const QMap<QString, ito::Param> &params, const QGLFormat &format, QWidget *parent = 0, const QGLWidget *shareWidget = 0, Qt::WindowFlags f = 0);
        ~PrjWindow();

        ito::RetVal calcLUT(QVector<double> *grayvalues, QVector<unsigned char> *lut);
        ito::RetVal setLUT(QVector<unsigned char> *lut);
        ito::RetVal setColor(const int col);
        ito::RetVal setGamma(const int gamma);
        ito::RetVal setGammaPrj(const int gammaCol);

        int getNumImages(void);
        int getOrientation(void) {return m_orientation;};
        int getPhaseShift(void)  {return m_phaShift;};
        int getNumGrayImages(void);
        int getCurImg(void) { return m_imgNum; }
		int getOrientationClearedCurImg(void);
        int getGrayBitsVert(void) { return m_grayBitsVert; }
        int getGrayBitsHoriz(void) { return m_grayBitsHoriz; }
        unsigned char ** getCosPtrVert(void) { return m_cosImgsVert; };
        unsigned char ** getCosPtrHoriz(void) { return m_cosImgsHoriz; };
        unsigned char ** getGrayPtrVert(void) { return m_grayImgsVert; };
        unsigned char ** getGrayPtrHoriz(void) { return m_grayImgsHoriz; };

        enum InitState
        {
            unInit = 0,
            paramsValid  = 1,
            cosIsInit = 2,
            grayIsInit = 4,
            idleState = paramsValid | cosIsInit | grayIsInit,
            initFail = 128
        };

    private:
        QGLFormat::OpenGLVersionFlags m_glVer;
        int m_isInit;
        int m_color;
        int m_grayBitsVert;
        int m_grayBitsHoriz;
        int m_phaShift;
        int m_period;
        int m_orientation;
        int m_gamma;
        int m_imgNum;
        int m_direction;
        int m_gammaCol;
        GLuint m_texture[38];
        GLuint m_lutTex;
        unsigned char **m_cosImgsVert;
        unsigned char **m_cosImgsHoriz;
        unsigned char **m_grayImgsVert;
        unsigned char **m_grayImgsHoriz;
        QVector<unsigned char> m_lut;
        void paintGL();
        void paintEvent(QPaintEvent *pevent);
        void resizeEvent(QResizeEvent *pevent);

        GLuint ProgramName;
        GLuint ArrayBufferName;
        GLuint ElementBufferName;
        GLint UniformMVP;
        GLint UniformTexture;
        GLint UniformLut;
        GLint UniformGamma;
        GLint UniformColor;
        
        // Init Function
        ito::RetVal cosineInit();
        ito::RetVal graycodeInit();
        ito::RetVal setupProjection();

        //ito::RetVal setPeriod(const int period);
        //ito::RetVal setPhaseShift(const int phaseshift);
        //ito::RetVal setOrientation(const int orient);

        // Deleter Function
        ito::RetVal cosineExit();
        ito::RetVal graycodeExit();

    signals:
        void numberOfImagesChanged(int numImg, int numGray, int numCos); /** Signal to sent new max image counts to the parent-Plugin*/

    public slots:
        void setSize(int sizex, int sizey, bool reCalcGL = true);
        void setPos(int xpos, int ypos);

        ito::RetVal shotDown(ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal configProjection(int period, int phaseShift, int orient, ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal configProjectionFull(int xpos, int sizex, int ypos, int sizey, int period, int phaseShift, int orient, ItomSharedSemaphore *waitCond = NULL);

        ito::RetVal setOrientation(const int orient);

        void enableInit() { if (!(m_isInit & paramsValid)) m_isInit |= paramsValid; };
        void disableInit() { m_isInit &= ~paramsValid; };

        ito::RetVal showFirstImg(ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal showNextImg(ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal showFirstGrayImg(ItomSharedSemaphore *waitCond = NULL);
        ito::RetVal showFirstCosImg(ItomSharedSemaphore *waitCond = NULL);

        ito::RetVal showImageNum(const int num);

    private slots:

};

//----------------------------------------------------------------------------------------------------------------------------------

#endif // PROJWINDOW_H