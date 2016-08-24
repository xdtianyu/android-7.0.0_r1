package android.databinding.tool.writer;

class DynamicUtilWriter() {
    public fun write(targetSdk : kotlin.Int) : KCode = kcode("package android.databinding;") {
        nl("")
        nl("import android.os.Build.VERSION;")
        nl("import android.os.Build.VERSION_CODES;")
        nl("")
        nl("public class DynamicUtil {")
        tab("@SuppressWarnings(\"deprecation\")")
        tab("public static int getColorFromResource(final android.view.View root, final int resourceId) {") {
            if (targetSdk >= 23) {
                tab("if (VERSION.SDK_INT >= VERSION_CODES.M) {") {
                    tab("return root.getContext().getColor(resourceId);")
                }
                tab("}")
            }
            tab("return root.getResources().getColor(resourceId);")
        }
        tab("}")
        nl("}")
    }
}