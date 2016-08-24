package android.icu.cts.tools;

import com.google.common.base.Joiner;
import com.android.compatibility.common.util.AbiUtils;
import org.w3c.dom.Attr;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

/**
 * Generates an XML file suitable for CTS version 1.
 *
 * <p>A lot of this code is copied from {@code tools/utils/DescriptionGenerator.java} and
 * {@code tools/utils/CollectAllTests.java}. Ideally, that code should have been refactored to make
 * it usable in this case but that would have taken quite a lot of time and given that CTS version 1
 * is going away anyway and CTS version 2 doesn't need an XML file this seemed like the best
 * approach.
 */
public class GenerateTestCaseXML {

  private static final String ATTRIBUTE_RUNNER = "runner";

  private static final String ATTRIBUTE_PACKAGE = "appPackageName";

  private static final String ATTRIBUTE_NS = "appNameSpace";

  static final String TAG_PACKAGE = "TestPackage";

  static final String TAG_SUITE = "TestSuite";

  static final String TAG_CASE = "TestCase";

  static final String TAG_TEST = "Test";

  static final String ATTRIBUTE_NAME_VERSION = "version";

  static final String ATTRIBUTE_VALUE_VERSION = "1.0";

  static final String ATTRIBUTE_NAME_FRAMEWORK = "AndroidFramework";

  static final String ATTRIBUTE_VALUE_FRAMEWORK = "Android 1.0";

  static final String ATTRIBUTE_NAME = "name";

  static final String ATTRIBUTE_ABIS = "abis";

  public static void main(String[] args) throws Exception {
    if (args.length != 3) {
      throw new IllegalStateException(
          "... <test-list-path> <architecture> <output-file-path");
    }

    String testListPath = args[0];
    String architecture = args[1];
    String outputFilePath = args[2];

    File testListFile = new File(testListPath);
    String abis = Joiner.on(" ").join(AbiUtils.getAbisForArch(architecture));
    File testCaseXML = new File(outputFilePath);

    TestSuite root = new TestSuite("");
    try (FileReader fileReader = new FileReader(testListFile);
         BufferedReader reader = new BufferedReader(fileReader)) {

      String line;
      while ((line = reader.readLine()) != null) {
        int index = line.indexOf('#');
        String className = line.substring(0, index);
        String methodName = line.substring(index + 1);

        root.addTest(className, methodName);
      }
    }

    Document mDoc = DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();

    Element testPackageElem = mDoc.createElement(TAG_PACKAGE);
    mDoc.appendChild(testPackageElem);

    setAttribute(testPackageElem, ATTRIBUTE_NAME_VERSION, ATTRIBUTE_VALUE_VERSION);
    setAttribute(testPackageElem, ATTRIBUTE_NAME_FRAMEWORK, ATTRIBUTE_VALUE_FRAMEWORK);
    setAttribute(testPackageElem, ATTRIBUTE_NAME, "CtsIcuTestCases");
    setAttribute(testPackageElem, ATTRIBUTE_RUNNER, "android.icu.cts.IcuTestRunnerForCtsV1");
    setAttribute(testPackageElem, ATTRIBUTE_PACKAGE, "android.icu.dev.test");
    setAttribute(testPackageElem, ATTRIBUTE_NS, "android.icu.cts");

    root.addChildInformation(testPackageElem, abis);

    Transformer t = TransformerFactory.newInstance().newTransformer();

    // enable indent in result file
    t.setOutputProperty("indent", "yes");
    t.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "4");

    File directory = testCaseXML.getParentFile();
    if (!directory.exists() && !directory.mkdirs()) {
      throw new IOException("Could not create directory: " + directory);
    }

    t.transform(new DOMSource(mDoc), new StreamResult(new FileOutputStream(testCaseXML)));
  }

  /**
   * Set the attribute of element.
   *
   * @param elem The element to be set attribute.
   * @param name The attribute name.
   * @param value The attribute value.
   */
  protected static void setAttribute(Node elem, String name, String value) {
    Attr attr = elem.getOwnerDocument().createAttribute(name);
    attr.setNodeValue(value);

    elem.getAttributes().setNamedItem(attr);
  }


  /**
   * The contents of a {@link TestSuite}, may be a {@link TestSuite} or a {@link TestCase}.
   */
  private static abstract class SuiteContent {

    private final String name;

    private SuiteContent(String name) {
      this.name = name;
    }

    public String getName() {
      return name;
    }

    public abstract void addInformation(Element parent, String abis);
  }

  public static class TestSuite extends SuiteContent {

    private Map<String, SuiteContent> name2Content = new TreeMap<>();

    public TestSuite(String name) {
      super(name);
    }

    public TestSuite getSuite(String name) {
      return getSuiteContent(TestSuite.class, name);
    }

    public TestCase getTestCase(String name) {
      return getSuiteContent(TestCase.class, name);
    }

    private <S extends SuiteContent> S getSuiteContent(Class<? extends S> contentClass,
        String name) {
      SuiteContent content = name2Content.get(name);
      S s;
      if (content == null) {
        try {
          s = contentClass.getConstructor(String.class).newInstance(name);
        } catch (Exception e) {
          throw new RuntimeException("Could not create instance of " + contentClass, e);
        }
        name2Content.put(name, s);
      } else if (contentClass.isInstance(content)) {
        s = contentClass.cast(content);
      } else {
        throw new IllegalStateException("Expected " + this
            + " to have a TestSuite called '" + name + "' but has "
            + content + " instead");
      }
      return s;
    }

    public void addTest(String className, String methodName) {
      int index = className.indexOf('.');
      if (index == -1) {
        TestCase testCase = getTestCase(className);
        testCase.addMethod(methodName);
      } else {
        String suiteName = className.substring(0, index);
        TestSuite childSuite = getSuite(suiteName);
        childSuite.addTest(className.substring(index + 1), methodName);
      }
    }

    @Override
    public void addInformation(Element parent, String abis) {
      Element suiteElement = appendElement(parent, TAG_SUITE);

      setAttribute(suiteElement, ATTRIBUTE_NAME, getName());

      addChildInformation(suiteElement, abis);
    }

    public void addChildInformation(Element parent, String abis) {
      for (SuiteContent suiteContent : name2Content.values()) {
        suiteContent.addInformation(parent, abis);
      }
    }
  }

  public static class TestCase extends SuiteContent {

    private final Set<String> methods = new TreeSet<>();

    public TestCase(String name) {
      super(name);
    }

    @Override
    public void addInformation(Element parent, String abis) {
      Element testCaseElement = appendElement(parent, TAG_CASE);
      setAttribute(testCaseElement, ATTRIBUTE_NAME, getName());
      setAttribute(testCaseElement, ATTRIBUTE_ABIS, abis);

      for (String method : methods) {
        Element testElement = appendElement(testCaseElement, TAG_TEST);
        setAttribute(testElement, ATTRIBUTE_NAME, method);
      }
    }

    public void addMethod(String methodName) {
      methods.add(methodName);
    }
  }

  private static Element appendElement(Element parent, String tagName) {
    Element testCaseElement = parent.getOwnerDocument().createElement(tagName);
    parent.appendChild(testCaseElement);
    return testCaseElement;
  }
}
