#ifndef BENCH_GL_ALL_TESTS_H_
#define BENCH_GL_ALL_TESTS_H_

namespace glbench {

class TestBase;

TestBase* GetAttributeFetchShaderTest();
TestBase* GetClearTest();
TestBase* GetContextTest();
TestBase* GetFboFillRateTest();
TestBase* GetFillRateTest();
TestBase* GetReadPixelTest();
TestBase* GetSwapTest();
TestBase* GetTextureReuseTest();
TestBase* GetTextureUpdateTest();
TestBase* GetTextureUploadTest();
TestBase* GetTriangleSetupTest();
TestBase* GetVaryingsAndDdxyShaderTest();
TestBase* GetWindowManagerCompositingTest(bool scissor);
TestBase* GetYuvToRgbTest();

} // namespace glbench

#endif // BENCH_GL_ALL_TESTS_H_
