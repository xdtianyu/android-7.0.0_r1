/*
 * Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#define ALLOC_COUNT_MAX        256
#define ALLOC_SIZE_MAX         8000000
#define REALLOCS_COUNT         10000
#define PINNING_PERCENTAGE     0
#define DEFAULT_LOOP_COUNT     100
#define DEFAULT_CARVEOUT_AMT   80
#define SYS_ROOT               "/sys/devices/platform/tegra-nvmap/misc/nvmap"
#define WINDOW_WIDTH           1024
#define WINDOW_HEIGHT          768

/* GART is 32mb, but is limited to 24mb per process */
#define GART_FILL_SIZE         24

static int verbose = 0;
static const char space[] = "************************************************";
static GLuint vsObj, fsObj, programObj;
static Display *x_display;
static Window win;
static EGLDisplay egl_display;
static EGLContext egl_context;
static EGLSurface egl_surface;

const char vertex_src [] =
"                                               \
uniform mat4 transformMatrix;                   \
attribute vec4 position;                        \
attribute vec4 tcoord;                          \
varying vec2 st;                                \
                                                \
void main()                                     \
{                                               \
    gl_Position = transformMatrix * position;   \
    st = tcoord.st;                             \
}                                               \
";

const char fragment_src [] =
"                                               \
precision highp float;                          \
uniform sampler2D tex;                          \
varying vec2 st;                                \
                                                \
void main()                                     \
{                                               \
    gl_FragColor = texture2D(tex, st);          \
}                                               \
";

static const GLfloat sVertData[] = {
        -1, -1, 0, 1,
        1, -1, 0, 1,
        -1,  1, 0, 1,
        1,  1, 0, 1
};

/*
 * This function gets the available amount of carveout from sysfs.
 */
int GetCarveoutTotalSize(unsigned int *total)
{
        FILE* f;
        char buffer[256];

        sprintf(buffer, "%s/heap-generic-0/total_size", SYS_ROOT);

        f = fopen(buffer, "r");
        if(!f) {
            printf("Failed to open %s", buffer);
            return -1;
        }

        fscanf(f, "%d", total);
        fclose(f);

        return 0;
}

/*
 * This function gets the free amount of carveout from sysfs.
 */
int GetCarveoutFreeSize(unsigned int *free)
{
        FILE* f;
        char buffer[256];

        sprintf(buffer, "%s/heap-generic-0/free_size", SYS_ROOT);

        /*
         * Make sure all previous rendering calls have completed so we
         * can query free carveout size
         */
        glFinish();

        f = fopen(buffer, "r");
        if(!f) {
            printf("Failed to open %s", buffer);
            return -1;
        }

        fscanf(f, "%d", free);
        fclose(f);

        return 0;
}

/*
 * This function creates an RGBA texture with a given width and height.
 * Return value: handle to texture
 */
static GLuint CreateTexture(int width, int height, int number)
{
        char *data = NULL;
        int x, y, bytesPerPixel;
        GLuint tex;

        assert(number == (number & 0xF));

        /* There are 4 bytes per pixel for GL_RGBA & GL_UNSIGNED_BYTE */
        bytesPerPixel = 4;

        data = (char *)malloc((size_t)(width*height*bytesPerPixel));
        if (!data)
                return -1;

        for (x = 0; x < width; x++) {
                for (y = 0 ; y < height; y++) {
                        int idx = (y*width + x)*bytesPerPixel;
                        data[idx] = (number * 0xF) & 0xFF;
                        data[idx+1] = (number * 0xF) & 0xFF;
                        data[idx+2] = 0xFF;
                        data[idx+3] = 0xFF;
                }
        }

        /* create texture */
        glGenTextures(1, &tex);
        if (glGetError() != GL_NO_ERROR)
                goto fail;

        glActiveTexture(GL_TEXTURE0);
        if (glGetError() != GL_NO_ERROR)
                goto fail;

        glBindTexture(GL_TEXTURE_2D, tex);
        if (glGetError() != GL_NO_ERROR)
                goto fail;

        glTexImage2D(
                /* target */            GL_TEXTURE_2D,
                /* level */             0,
                /* internalformat */    (GLint)GL_RGBA,
                /* width */             width,
                /* height */            height,
                /* border */            0,
                /* format */            GL_RGBA,
                /* type */              GL_UNSIGNED_BYTE,
                /* pixels */            data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        goto done;

fail:
        tex = -1;
done:
        free(data);

        return tex;
}

/*
 * Fill in the result array with an identity matrix.
 */
static void LoadIdentity(GLfloat *result)
{
        memset(result, 0x0, 16*4);
        result[0] = 1;
        result[5] = 1;
        result[10] = 1;
        result[15] = 1;
}

/*
 * Fill in the result array with a scaling matrix.
 */
static void Scale(GLfloat *result, GLfloat sx, GLfloat sy, GLfloat sz)
{
        result[0] *= sx;
        result[1] *= sx;
        result[2] *= sx;
        result[3] *= sx;

        result[4] *= sy;
        result[5] *= sy;
        result[6] *= sy;
        result[7] *= sy;

        result[8] *= sz;
        result[9] *= sz;
        result[10] *= sz;
        result[11] *= sz;
}

/*
 * Fill in the result array with a transformation matrix.
 */
static void Translate(GLfloat *result, GLfloat tx, GLfloat ty, GLfloat tz)
{
        result[12] += (result[0] * tx + result[4] * ty + result[8] * tz);
        result[13] += (result[1] * tx + result[5] * ty + result[9] * tz);
        result[14] += (result[2] * tx + result[6] * ty + result[10] * tz);
        result[15] += (result[3] * tx + result[7] * ty + result[11] * tz);
}

/*
 * This function takes a given texture array and displays the textures in it.
 * All textures need to be the same size, width x height.
 */
int ShowTextures(GLuint *tex, int count, int width, int height)
{
        GLint texSampler;
        GLint transformMatrixUniform;
        GLfloat vertSTData[8];
        int i;
        GLfloat transformMatrix[16];
        int cols = (int)sqrtf(count);
        struct timeval tv;
        int rnd;

        gettimeofday(&tv, NULL);
        rnd = tv.tv_sec * 1000;

        /* Texture coords */
        vertSTData[0] = 0;
        vertSTData[1] = 0;
        vertSTData[2] = width;
        vertSTData[3] = 0;
        vertSTData[4] = 0;
        vertSTData[5] = height;
        vertSTData[6] = width;
        vertSTData[7] = height;

        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, vertSTData);
        texSampler = glGetUniformLocation(programObj, "tex");
        transformMatrixUniform = glGetUniformLocation(programObj,
                                        "transformMatrix");
        glUniform1i(texSampler, 0);

        /* Draw texture rectangles */
        LoadIdentity(transformMatrix);
        Scale(transformMatrix, 4.0f/cols, 4.0f/cols, 4.0f/cols);
        Translate(transformMatrix, -cols - 1.0f, cols - 1.0f, 0.0f);
        for (i = 0; i < count; i++) {
            rnd = rnd * 69069 + 69069;
            if(((rnd / 1217) & 255) > 128) {
                Translate(transformMatrix, 2.0f, 0.0f, 0.0f);
                glUniformMatrix4fv(transformMatrixUniform, 1, GL_FALSE,
                                        transformMatrix);
                glBindTexture(GL_TEXTURE_2D, tex[i]);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                if (((i+1) % cols) == 0) {
                    Translate(transformMatrix, -2.0f*cols, -2.0f,
                                0.0f);
                }
            }
        }

        /* Issue flush to ensure all gl commands are sent to processing */
        glFlush();

        return 0;
}

/*
 * This function runs a single phase of the test.
 * Return value: 0 on success
 */
int RunPhase(int phase, int phase_width, int phase_height, int texture_count,
                int texture_size, GLuint *tex)
{
        unsigned int phase_size = texture_count * texture_size;
        unsigned int freeBefore, freeAfter;
        GLenum errCode = GL_NO_ERROR;
        int brokeAt = 0;
        unsigned int displacement;
        int i;

        printf("%.*s\n", 48, space);
        printf("Phase %d: Allocating small %d (%dMB) [%dMB] textures\n",
                        phase, texture_count, texture_size,
                        texture_count * texture_size);
        printf("%.*s\n", 48, space);

        /* Create texture */
        printf("allocating textures.. (%d,%d)\n", phase_width, phase_height);
        GetCarveoutFreeSize(&freeBefore);
        for (i = 0; i < texture_count; i++) {
                tex[i] = CreateTexture(phase_width - 1, phase_height - 1,
                                        (i % 16));

                if (tex[i] < 0) {
                        printf("Failed to create texture.\n");
                        brokeAt = i;
                        goto done;
                }

                errCode = glGetError();
                if (errCode != GL_NO_ERROR) {
                        printf("GL Error Occured : %d\n", errCode);
                        brokeAt = i;
                        goto done;
                }
        }

        GetCarveoutFreeSize(&freeAfter);
        /* Calculate difference in MB */
        displacement = (freeBefore - freeAfter) / (1024 * 1024);

        if (displacement < phase_size) {
                fprintf(stderr, "FAIL to alloc required mem from carveout.\n");
                fprintf(stderr, "Allocated %dMB instead of desired %dMB\n",
                        displacement, phase_size);
                brokeAt = texture_count - 1;
                goto done;
        }

        if (verbose) {
                unsigned int free;
                GetCarveoutFreeSize(&free);
                printf("CarveOut free after phase %d allocation: %d\n",
                        phase, free);
        }

done:
        return brokeAt; /* brokeAt is 0 unless and error happened */
}

/*
 * This function runs the actual test, drawing the texture rects.
 * Return value: 0 on success
 */
int RunTest(int carveout_percent)
{
        GLuint phase1_tex[ALLOC_COUNT_MAX];
        GLuint phase2_tex[ALLOC_COUNT_MAX / 4];
        GLuint phase3_tex[ALLOC_COUNT_MAX / 8];
        int i;
        unsigned int allocCount = 0;
        unsigned int carveoutTotal;
        unsigned int carveoutFree;
        int allocatedMemoryLimit;
        GLenum errCode = GL_NO_ERROR;
        int phase = 0;
        int brokeAt = 0;

        if(GetCarveoutTotalSize(&carveoutTotal) ||
           GetCarveoutFreeSize(&carveoutFree)) {
                printf("Failed to read carveout stats\n");
                return -1;
        }

        printf("CarveOut total before cleanup: %d [%dMB]\n", carveoutTotal,
                carveoutTotal / (1024*1024));
        printf("CarveOut free before cleanup: %d [%dMB]\n", carveoutFree,
                carveoutFree / (1024*1024));

        /* Memory is in units of bytes */

        allocatedMemoryLimit = (int)((carveoutFree / 100) * carveout_percent);
        allocCount = allocatedMemoryLimit / 1048576; /* 1 mb */
        allocCount = (allocCount / 4) * 4; /* always a multiple of 4. */

        phase = 1;
        errCode = RunPhase(phase, 512, 512, allocCount, 1, phase1_tex);
        if (errCode) {
                brokeAt = errCode;
                goto cleanup_phase1;
        }

        printf("freeing first 3 of every 4 textures from phase 1 [%dMB]\n",
                        (allocCount * 3 / 4) * 1);
        for (i = 0; i < allocCount; i++) {
                if ((i + 1) % 4 == 0) continue;
                glDeleteTextures(1, &phase1_tex[i]);
        }

        /*
         * Make sure all previous rendering calls have completed so we
         * can query free carveout size
         */
        glFinish();

        if (verbose) {
                unsigned int free;
                GetCarveoutFreeSize(&free);
                printf("CarveOut free after phase 1 freeing: %d\n", free);
        }

        /*
         * prepare for phase 2
         * we free'd up 3/4 of the phase 1 handles, and then these will be
         * 4x as large, so the number will further shrink so 3/16
         */
        allocCount = (allocCount * 3) / 16;
        phase = 2;
        errCode = RunPhase(phase, 1024, 1024, allocCount, 4, phase2_tex);
        if (errCode) {
                brokeAt = errCode;
                goto cleanup_phase2;
        }

        printf("freeing every other texture from phase 2 [%dMB]\n",
                        (allocCount / 2) * 4 );
        for (i = 0; i < allocCount; i++) {
                if (i % 2 != 0) continue;
                glDeleteTextures(1, &phase2_tex[i]);
        }

        /*
         * Make sure all previous rendering calls have completed so we
         * can query free carveout size
         */
        glFinish();

        if (verbose) {
                unsigned int free;
                GetCarveoutFreeSize(&free);
                printf("CarveOut free after phase 2 freeing: %d\n", free);
        }

        /*
         * prepare for phase 3
         * we free'd up 1/2 of the phase 2 handles, and then these will be
         * 2x as large, so the number will further shrink so 1/4
         */
        allocCount = (allocCount / 4);
        phase = 3;
        errCode = RunPhase(phase, 2048, 1024, allocCount, 8, phase3_tex);
        if (errCode) {
                brokeAt = errCode;
                goto cleanup_phase3;
        }

        printf("%.*s\n", 48, space);
        printf("Test Complete \n");
        printf("%.*s\n", 48, space);

cleanup_phase3:
        printf("freeing last textures from phase 3\n");
        for (i = 0; i < (brokeAt ? brokeAt : allocCount); i++)
                glDeleteTextures(1, &phase3_tex[i]);

cleanup_phase2:
        printf("freeing last textures from phase 2\n");
        if (phase == 2 && errCode != GL_NO_ERROR)
                for (i = 0; i < (brokeAt ? brokeAt : allocCount); i++)
                        glDeleteTextures(1, &phase2_tex[i]);
        else
                for (i = 0; i < allocCount; i += 2)
                        glDeleteTextures(1, &phase2_tex[i]);

cleanup_phase1:
        printf("freeing last textures from phase 1\n");
        if (phase == 1 && errCode != GL_NO_ERROR)
                for (i = 0; i < (brokeAt ? brokeAt : allocCount); i++)
                        glDeleteTextures(1, &phase1_tex[i]);
        else
                for (i = 3; i < allocCount; i += 4)
                        glDeleteTextures(1, &phase1_tex[i]);

        return (errCode == GL_NO_ERROR ? 0 : -1);
}

/*
 * This function prints the info log for a given shader (from handle).
 */
void PrintShaderInfoLog(GLuint shader)
{
        GLint        length;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

        if (length) {
                char buffer[length];
                glGetShaderInfoLog(shader, length, NULL, buffer);
                printf("shader info: %s\n", buffer);
        }
}

GLuint LoadShader(const char *shader_src, GLenum type)
{
        GLuint        shader = glCreateShader(type);
        GLint         success;

        glShaderSource(shader, 1, &shader_src, NULL);
        glCompileShader(shader);
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (success != GL_TRUE) {
                fprintf(stderr, "FAILED to compile shader. %d\n", success);
                return success;
        }

        if (verbose)
                PrintShaderInfoLog(shader);

        return shader;
}

static void InitGraphicsState()
{
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, sVertData);
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);

        vsObj = LoadShader(vertex_src, GL_VERTEX_SHADER);
        fsObj = LoadShader(fragment_src, GL_FRAGMENT_SHADER);

        programObj = glCreateProgram();
        glAttachShader(programObj, vsObj);
        glAttachShader(programObj, fsObj);
        glBindAttribLocation(programObj, 0, "position");
        glBindAttribLocation(programObj, 1, "tcoord");
        glLinkProgram(programObj);
        glUseProgram(programObj);

        /* so that odd-sized RGB textures will work nicely */
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        glDisable(GL_DEPTH_TEST);
}

void CleanupX()
{
        XDestroyWindow(x_display, win);
        XCloseDisplay(x_display);
}

void CleanupEgl()
{
        eglDestroyContext(egl_display, egl_context);
        eglDestroySurface(egl_display, egl_surface);
        eglTerminate(egl_display);
}

int XInitialize(int x, int y, int width, int height)
{
        Window                         root;
        XSetWindowAttributes         swa;
        XSetWindowAttributes         xattr;
        Atom                         atom;
        XWMHints                     hints;
        int                             xres;

        x_display = XOpenDisplay(NULL);
        if (x_display == NULL) {
                printf("Cannot connect to X server. Exiting...\n");
                return -1;
        }

        root = DefaultRootWindow(x_display);
        swa.event_mask = ExposureMask | PointerMotionMask | KeyPressMask;

        if (verbose)
                printf("Creating window at (%d,%d) with w=%d, h=%d\n",
                        x, y, width, height);

        win = XCreateWindow(
                /* connection to x server */      x_display,
                /* parent window */               root,
                /* x coord, top left corner */    x,
                /* y coord, top left corner */    y,
                /* width of window */             width,
                /* height of window */            height,
                /* border width */                0,
                /* depth of window */             CopyFromParent,
                /* window's class */              InputOutput,
                /* visual type */                 CopyFromParent,
                /* valid attribute mask */        CWEventMask,
                /* attributes */                  &swa);
        if (win == BadAlloc ||
            win == BadColor ||
            win == BadCursor ||
            win == BadMatch ||
            win == BadPixmap ||
            win == BadValue ||
            win == BadWindow) {
                fprintf(stderr, "FAILED to create X window\n");
                return -1;
        }

        xattr.override_redirect = false;
        xres = XChangeWindowAttributes(x_display, win, CWOverrideRedirect,
                                        &xattr);
        if (xres == BadAccess ||
            xres == BadColor ||
            xres == BadCursor ||
            xres == BadMatch ||
            xres == BadPixmap ||
            xres == BadValue ||
            xres == BadWindow) {
                fprintf(stderr, "FAIL changing X win attribs: %d\n", xres);
                goto fail;
        }

        atom = XInternAtom(x_display, "_NET_WM_STATE_FULLSCREEN", true);

        hints.input = true;
        hints.flags = InputHint;
        xres = XSetWMHints(x_display, win, &hints);
        if (xres == BadAlloc || xres == BadWindow) {
                fprintf(stderr, "FAIL setting X WM hints: %d\n", xres);
                goto fail;
        }

        xres = XMapWindow(x_display, win);
        if (xres == BadAlloc || xres == BadWindow ) {
                fprintf(stderr, "FAIL mapping X window: %d\n", xres);
                goto fail;
        }

        xres = XStoreName(x_display, win, "GLES2 Texture Test");
        if (xres == BadAlloc || xres == BadWindow) {
                fprintf(stderr, "FAIL storing X window name: %d\n", xres);
                goto fail;
        }

        return 0;

fail:
        CleanupX();

        return -1;
}


int EglInitialize()
{
        EGLConfig        config;
        EGLint                numConfig;

        EGLint        attr[] = {
                EGL_BUFFER_SIZE, 16,
                EGL_RENDERABLE_TYPE,
                EGL_OPENGL_ES2_BIT,
                EGL_NONE
        };

        EGLint ctxattr[] = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL_NONE
        };

        egl_display = eglGetDisplay((EGLNativeDisplayType)x_display);
        if (egl_display == EGL_NO_DISPLAY) {
                fprintf(stderr, "EGL failed to obtain display. Exiting...\n");
                return -1;
        }

        if ( !eglInitialize(egl_display, NULL, NULL)) {
                fprintf(stderr, "EGL failed to initialize. Exiting...\n");
                return -1;
        }

        if ( !eglChooseConfig(egl_display, attr, &config, 1, &numConfig)) {
                fprintf(stderr, "EGL failed to choose config. Exiting...\n");
                return -1;
        }

        if (numConfig != 1) {
                fprintf(stderr, "EGL failed got %d > 1 Exiting...\n",
                        numConfig);
                return -1;
        }

        egl_surface = eglCreateWindowSurface(egl_display, config, win, NULL);
        if (egl_surface == EGL_NO_SURFACE) {
                fprintf(stderr, "EGL failed create window surface. Exiting\n");
                return -1;
        }

        egl_context = eglCreateContext(egl_display, config, EGL_NO_CONTEXT,
                                        ctxattr);
        if (egl_context == EGL_NO_CONTEXT) {
                fprintf(stderr, "EGL failed to create context. Exiting...\n");
                return -1;
        }

        if ( !eglMakeCurrent(egl_display, egl_surface, egl_surface,
                                egl_context)) {
                fprintf(stderr, "EGL failed make context current. Exiting\n");
                return -1;
        }

        return 0;
}

void PrintUsage()
{
        printf("--------------------------------------------\n");
        printf("nvmap_carveout_compactor [options]\n");
        printf("  -h | --help              - Show this help screen\n");
        printf("  -v | --verbose           - Enables verbose prints\n");
        printf("  -l | --loop_count        - # of times to loop [def: %d]\n",
                DEFAULT_LOOP_COUNT);
        printf("  -c | --carveout_percent  - %% of free carveout [def : %d].\n",
                DEFAULT_CARVEOUT_AMT);
}

int main(int argc, char *argv[])
{
        GLuint fill_tex[GART_FILL_SIZE];
        int failure = 0;
        int failIndex = 0;
        int i;
        int loop_count = 1;
        int option_index = 0;
        GLenum errCode;
        int x = 0, y = 0, width = WINDOW_WIDTH, height = WINDOW_HEIGHT;
        int carveout_percent = 80; /* 80 percent of free carveout */

        static struct option long_options[] = {
                {"help",             no_argument,       0,        'h'},
                {"verbose",          no_argument,       0,        'v'},
                {"loop_count",       required_argument, 0,        'l'},
                {"carveout_percent", required_argument, 0,        'c'},
                {NULL,               0,                 NULL,     0}
        };

        if (!getenv("DISPLAY")) {
                fprintf(stderr, "FAIL: DISPLAY env variable not set.\n");
                failure = -1;
                goto done;
        }

        while ((i = getopt_long(argc, argv, "hvl:c:", long_options,
                        &option_index)) != -1)
                switch (i) {
                        case 'h':
                                PrintUsage();
                                return 0;
                        case 'v':
                                verbose = 1;
                                break;
                        case 'l':
                                loop_count = atoi(optarg);
                                break;
                        case 'c':
                                carveout_percent = atoi(optarg);
                                break;
                        case '?':
                                printf("unknown option `\\x%x`.\n", optopt);
                                return 1;
                        default:
                                goto done;
                }

        failure = XInitialize(x, y, width, height);
        if (failure)
                goto done;

        failure = EglInitialize();
        if (failure)
                goto clean_x;

        InitGraphicsState();

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        printf("Test started pid = %d.\n", getpid());
        if (verbose) {
                printf("Looping for %d iterations.\n", loop_count);
                printf("Going to try to use %d%% of carveout during test.\n",
                        carveout_percent);
        }

        /* first need to allocate 24mb of textures to fill GART */
        printf("allocating textures to fill GART.. (%d,%d)\n",
                512, 512);

        /* Each process gets 24mb of GART */
        for (i = 0; i < GART_FILL_SIZE; i++) {
                fill_tex[i] = CreateTexture(511, 511, (i % 16));

                errCode = glGetError();
                if (fill_tex[i] >= 0 && errCode == GL_NO_ERROR)
                        continue;

                /* Some error occured when creating textures */
                printf("Failed to create texture.\n");
                if (errCode != GL_NO_ERROR)
                               printf("GL Error Occured : %d\n", errCode);

                failIndex = i;
                failure = -1;
                goto done;
        }

        ShowTextures(fill_tex, GART_FILL_SIZE, 512, 512);

        /* predefined resolutions to account for size */

        for(i = 0; i < loop_count; i++) {
                failure |= RunTest(carveout_percent);
                eglSwapBuffers(egl_display, egl_surface);
        }

        if (!failure) {
                errCode = glGetError();
                if (errCode == GL_NO_ERROR)
                        failure = false;
                else {
                        fprintf(stderr, "FAIL: GL Error Occured : %d\n",
                                errCode);
                        failure = 1;
                }
        }

        CleanupEgl();
clean_x:
        CleanupX();
done:

        for (i = 0; i < (failIndex ? failIndex : GART_FILL_SIZE); i++)
                glDeleteTextures(1, &fill_tex[i]);

        if (!failure)
                printf("Test completed [SUCCESS]: pid = %d\n", getpid());
        else
                fprintf(stderr, "Test completed [FAIL]: pid = %d\n", getpid());

        return failure ? -1 : 0;
}

