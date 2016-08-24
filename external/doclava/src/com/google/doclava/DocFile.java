/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.doclava;

import com.google.clearsilver.jsilver.data.Data;

import java.io.*;
import java.util.regex.Pattern;
import java.util.regex.Matcher;


public class DocFile {
  public static final Pattern LINE = Pattern.compile("(.*)[\r]?\n", Pattern.MULTILINE);
  public static final Pattern PROP = Pattern.compile("([^=]+)=(.*)");

  public static String readFile(String filename) {
    try {
      File f = new File(filename);
      int length = (int) f.length();
      FileInputStream is = new FileInputStream(f);
      InputStreamReader reader = new InputStreamReader(is, "UTF-8");
      char[] buf = new char[length];
      int index = 0;
      int amt;
      while (true) {
        amt = reader.read(buf, index, length - index);

        if (amt < 1) {
          break;
        }

        index += amt;
      }
      return new String(buf, 0, index);
    } catch (IOException e) {
      return null;
    }
  }

  public static String[] DEVSITE_VALID_LANGS = {"en", "es", "in", "ja", "ko",
      "ru", "vi", "zh-cn", "zh-tw", "pt-br"};

  public static String getPathRoot(String filename) {
    //look for a valid lang string in the file path. If found,
    //snip the intl/lang from the path.
    for (String t : DEVSITE_VALID_LANGS) {
      int langStart = filename.indexOf("/" + t + "/");
      if (langStart > -1) {
        int langEnd = filename.indexOf("/", langStart + 1);
        filename = filename.substring(langEnd + 1);
        break;
      }
    }
    return filename;
  }

  public static Data getPageMetadata (String docfile, Data hdf) {
    //utility method for extracting metadata without generating file output.
    if (hdf == null) {
      hdf = Doclava.makeHDF();
    }
    String filedata = readFile(docfile);

    // The document is properties up until the line "@jd:body".
    // Any blank lines are ignored.
    int start = -1;
    int lineno = 1;
    Matcher lines = LINE.matcher(filedata);
    String line = null;
    while (lines.find()) {
      line = lines.group(1);
      if (line.length() > 0) {
        if (line.equals("@jd:body")) {
          start = lines.end();
          break;
        }
        Matcher prop = PROP.matcher(line);
        if (prop.matches()) {
          String key = prop.group(1);
          String value = prop.group(2);
          hdf.setValue(key, value);
        } else {
          break;
        }
      }
      lineno++;
    }
    if (start < 0) {
      System.err.println(docfile + ":" + lineno + ": error parsing docfile");
      if (line != null) {
        System.err.println(docfile + ":" + lineno + ":" + line);
      }
      System.exit(1);
    }
    return hdf;
  }

  public static void writePage(String docfile, String relative, String outfile, Data hdf) {

    /*
     * System.out.println("docfile='" + docfile + "' relative='" + relative + "'" + "' outfile='" +
     * outfile + "'");
     */
    if (hdf == null) {
      hdf = Doclava.makeHDF();
    }
    String filedata = readFile(docfile);

    // The document is properties up until the line "@jd:body".
    // Any blank lines are ignored.
    int start = -1;
    int lineno = 1;
    Matcher lines = LINE.matcher(filedata);
    String line = null;
    while (lines.find()) {
      line = lines.group(1);
      if (line.length() > 0) {
        if (line.equals("@jd:body")) {
          start = lines.end();
          break;
        }
        Matcher prop = PROP.matcher(line);
        if (prop.matches()) {
          String key = prop.group(1);
          String value = prop.group(2);
          hdf.setValue(key, value);
        } else {
          break;
        }
      }
      lineno++;
    }
    if (start < 0) {
      System.err.println(docfile + ":" + lineno + ": error parsing docfile");
      if (line != null) {
        System.err.println(docfile + ":" + lineno + ":" + line);
      }
      System.exit(1);
    }

    // if they asked to only be for a certain template, maybe skip it
    String fromTemplate = hdf.getValue("template.which", "");
    String fromPage = hdf.getValue("page.onlyfortemplate", "");
    if (!"".equals(fromPage) && !fromTemplate.equals(fromPage)) {
      return;
    }

    // and the actual text after that
    String commentText = filedata.substring(start);

    Comment comment = new Comment(commentText, null, new SourcePositionInfo(docfile, lineno, 1));
    TagInfo[] tags = comment.tags();

    TagInfo.makeHDF(hdf, "root.descr", tags);

    hdf.setValue("commentText", commentText);

    // write the page using the appropriate root template, based on the
    // whichdoc value supplied by build
    String fromWhichmodule = hdf.getValue("android.whichmodule", "");
    if (fromWhichmodule.equals("online-pdk")) {
      // leaving this in just for temporary compatibility with pdk doc
      hdf.setValue("online-pdk", "true");
      // add any conditional login for root template here (such as
      // for custom left nav based on tab etc.
      ClearPage.write(hdf, "docpage.cs", outfile);
    } else {
      String filename = outfile;
      // Special case handling of samples files for devsite
      // locale handling -- strip out the en/ root
      if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
        filename = filename.replaceFirst("^en/", "");
      }
      // Strip out the intl and lang id substr and get back just the
      // guide, design, distribute, etc.
      filename = getPathRoot(filename);

      if (Doclava.USE_UPDATED_TEMPLATES) {
        //remap types to design, dev, distribute etc.
        if (filename.indexOf("design") == 0) {
          hdf.setValue("design", "true");
          hdf.setValue("page.type", "design");
          hdf.setValue("page.category", "design");
        } else if (filename.indexOf("develop") == 0) {
          hdf.setValue("develop", "true");
          hdf.setValue("page.type", "develop");
          hdf.setValue("page.category", "develop");
        } else if (filename.indexOf("guide") == 0) {
          hdf.setValue("guide", "true");
          hdf.setValue("page.type", "develop");
          if (filename.indexOf("guide/topics/manif") == 0) {
            hdf.setValue("page.category", "app manifest");
          } else {
            hdf.setValue("page.category", "guide");
          }
        } else if (filename.indexOf("training") == 0) {
          hdf.setValue("training", "true");
          hdf.setValue("page.type", "develop");
          hdf.setValue("page.category", "training");
        } else if (filename.indexOf("more") == 0) {
          hdf.setValue("more", "true");
        } else if (filename.indexOf("google") == 0) {
          hdf.setValue("google", "true");
          hdf.setValue("page.type", "develop");
          hdf.setValue("page.category", "google");
        } else if (filename.indexOf("samples") == 0) {
          hdf.setValue("samples", "true");
          hdf.setValue("samplesDocPage", "true");
          hdf.setValue("page.type", "develop");
          hdf.setValue("page.category", "samples");
          if (Doclava.samplesNavTree != null) {
            hdf.setValue("samples_toc_tree", Doclava.samplesNavTree.getValue("samples_toc_tree", ""));
          }
        } else if (filename.indexOf("topic/") == 0) {
          hdf.setValue("topic", "true");
          hdf.setValue("page.type", "develop");
          if (filename.indexOf("topic/libraries") == 0) {
            hdf.setValue("page.category", "libraries");
            hdf.setValue("page.type", "develop");
            hdf.setValue("libraries", "true");
          } else if (filename.indexOf("topic/instant-apps") == 0) {
            hdf.setValue("instantapps", "true");
            hdf.setValue("page.type", "develop");
            hdf.setValue("page.category", "instant apps");
          }
        } else if (filename.indexOf("distribute") == 0) {
          hdf.setValue("distribute", "true");
          hdf.setValue("page.type", "distribute");
          hdf.setValue("page.category", "distribute");
          if (filename.indexOf("distribute/googleplay") == 0) {
            hdf.setValue("page.category", "googleplay");
            hdf.setValue("page.type", "distribute");
            hdf.setValue("googleplay", "true");
          } else if (filename.indexOf("distribute/essentials") == 0) {
            hdf.setValue("page.category", "essentials");
            hdf.setValue("essentials", "true");
          } else if (filename.indexOf("distribute/users") == 0) {
            hdf.setValue("page.category", "users");
            hdf.setValue("users", "true");
          } else if (filename.indexOf("distribute/engage") == 0) {
            hdf.setValue("page.category", "engage");
            hdf.setValue("engage", "true");
          } else if (filename.indexOf("distribute/monetize") == 0) {
            hdf.setValue("page.category", "monetize");
            hdf.setValue("monetize", "true");
          } else if (filename.indexOf("distribute/analyze") == 0) {
            hdf.setValue("page.category", "analyze");
            hdf.setValue("analyze", "true");
          } else if (filename.indexOf("distribute/tools") == 0) {
            hdf.setValue("page.category", "essentials");
            hdf.setValue("essentials", "true");
          } else if (filename.indexOf("distribute/stories") == 0) {
            hdf.setValue("page.category", "stories");
            hdf.setValue("stories", "true");
          }
        } else if (filename.indexOf("about") == 0) {
          hdf.setValue("about", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "about");
          if ((filename.indexOf("about/versions") == 0)) {
            hdf.setValue("versions", "true");
            hdf.setValue("page.category", "versions");
          //todo remove this because there's no file at this location
          } else if ((filename.indexOf("wear") == 0)) {
            hdf.setValue("wear", "true");
            hdf.setValue("page.category", "wear");
          } else if ((filename.indexOf("tv") == 0)) {
            hdf.setValue("tv", "true");
            hdf.setValue("page.category", "tv");
          } else if ((filename.indexOf("auto") == 0)) {
            hdf.setValue("auto", "true");
            hdf.setValue("page.category", "auto");
          }
        } else if (filename.indexOf("wear/preview") == 0) {
          hdf.setValue("wearpreview", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "wear preview");
        } else if ((filename.indexOf("tools") == 0) || (filename.indexOf("sdk") == 0)) {
          hdf.setValue("tools", "true");
          hdf.setValue("page.type", "develop");
          hdf.setValue("page.category", "tools");
          fromTemplate = hdf.getValue("page.template", "");
        } else if (filename.indexOf("devices") == 0) {
          hdf.setValue("devices", "true");
          hdf.setValue("page.type", "devices");
        } else if (filename.indexOf("source") == 0) {
          hdf.setValue("source", "true");
        } else if (filename.indexOf("accessories") == 0) {
          hdf.setValue("accessories", "true");
        } else if (filename.indexOf("compatibility") == 0) {
          hdf.setValue("compatibility", "true");
        } else if (filename.indexOf("wear") == 0) {
          hdf.setValue("wear", "true");
          hdf.setValue("about", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "wear");
        } else if (filename.indexOf("work") == 0) {
          hdf.setValue("work", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "work");
        } else if (filename.indexOf("preview") == 0) {
          hdf.setValue("page.type", "develop");
          hdf.setValue("page.category", "preview");
          hdf.setValue("preview", "true");
        } else if (filename.indexOf("auto") == 0) {
          hdf.setValue("auto", "true");
          hdf.setValue("about", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "auto");
        } else if (filename.indexOf("tv") == 0) {
          hdf.setValue("tv", "true");
          hdf.setValue("about", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "tv");
        } else if (filename.indexOf("ndk") == 0) {
          hdf.setValue("ndk", "true");
          hdf.setValue("page.type", "ndk");
          hdf.setValue("page.category", "ndk");
          if (filename.indexOf("ndk/guides") == 0) {
            hdf.setValue("guide", "true");
            hdf.setValue("page.type", "ndk");
            hdf.setValue("page.category", "guide");
          } else if (filename.indexOf("ndk/reference") == 0) {
            hdf.setValue("reference", "true");
            hdf.setValue("page.type", "ndk");
            hdf.setValue("page.category", "reference");
          } else if (filename.indexOf("ndk/samples") == 0) {
            hdf.setValue("samples", "true");
            hdf.setValue("page.type", "ndk");
            hdf.setValue("page.category", "samples");
            hdf.setValue("samplesDocPage", "true");
          } else if (filename.indexOf("ndk/downloads") == 0) {
            hdf.setValue("downloads", "true");
            hdf.setValue("page.type", "ndk");
            hdf.setValue("page.category", "downloads");
            fromTemplate = hdf.getValue("page.template", "");
          }
        }
      } else {
        //support the old mappings
        if (filename.indexOf("design") == 0) {
          hdf.setValue("design", "true");
          hdf.setValue("page.type", "design");
        } else if (filename.indexOf("develop") == 0) {
          hdf.setValue("develop", "true");
          hdf.setValue("page.type", "develop");
        } else if (filename.indexOf("guide") == 0) {
          hdf.setValue("guide", "true");
          hdf.setValue("page.type", "guide");
        } else if (filename.indexOf("training") == 0) {
          hdf.setValue("training", "true");
          hdf.setValue("page.type", "training");
        } else if (filename.indexOf("more") == 0) {
          hdf.setValue("more", "true");
        } else if (filename.indexOf("google") == 0) {
          hdf.setValue("google", "true");
          hdf.setValue("page.type", "google");
        } else if (filename.indexOf("samples") == 0) {
          hdf.setValue("samples", "true");
          hdf.setValue("samplesDocPage", "true");
          hdf.setValue("page.type", "samples");
          if (Doclava.samplesNavTree != null) {
            hdf.setValue("samples_toc_tree", Doclava.samplesNavTree.getValue("samples_toc_tree", ""));
          }
        } else if (filename.indexOf("distribute") == 0) {
          hdf.setValue("distribute", "true");
          hdf.setValue("page.type", "distribute");
          if (filename.indexOf("distribute/googleplay") == 0) {
            hdf.setValue("googleplay", "true");
          } else if (filename.indexOf("distribute/essentials") == 0) {
            hdf.setValue("essentials", "true");
          } else if (filename.indexOf("distribute/users") == 0) {
            hdf.setValue("users", "true");
          } else if (filename.indexOf("distribute/engage") == 0) {
            hdf.setValue("engage", "true");
          } else if (filename.indexOf("distribute/monetize") == 0) {
            hdf.setValue("monetize", "true");
          } else if (filename.indexOf("distribute/analyze") == 0) {
            hdf.setValue("analyze", "true");
          } else if (filename.indexOf("distribute/tools") == 0) {
            hdf.setValue("essentials", "true");
          } else if (filename.indexOf("distribute/stories") == 0) {
            hdf.setValue("stories", "true");
          }
        } else if (filename.indexOf("about") == 0) {
          hdf.setValue("about", "true");
          hdf.setValue("page.type", "about");
        } else if ((filename.indexOf("tools") == 0) || (filename.indexOf("sdk") == 0)) {
          hdf.setValue("tools", "true");
          hdf.setValue("page.type", "tools");
          fromTemplate = hdf.getValue("page.template", "");
        } else if (filename.indexOf("devices") == 0) {
          hdf.setValue("devices", "true");
          hdf.setValue("page.type", "devices");
        } else if (filename.indexOf("source") == 0) {
          hdf.setValue("source", "true");
        } else if (filename.indexOf("accessories") == 0) {
          hdf.setValue("accessories", "true");
        } else if (filename.indexOf("compatibility") == 0) {
          hdf.setValue("compatibility", "true");
        } else if (filename.indexOf("topic/") == 0) {
          hdf.setValue("topic", "true");
          hdf.setValue("page.type", "develop");
          if (filename.indexOf("topic/libraries") == 0) {
            hdf.setValue("page.category", "libraries");
            hdf.setValue("page.type", "develop");
            hdf.setValue("libraries", "true");
          } else if (filename.indexOf("topic/instant-apps") == 0) {
            hdf.setValue("instantapps", "true");
            hdf.setValue("page.type", "develop");
            hdf.setValue("page.category", "instant apps");
          }
        } else if (filename.indexOf("wear/preview") == 0) {
          hdf.setValue("wearpreview", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "wear preview");
        } else if (filename.indexOf("wear") == 0) {
          hdf.setValue("wear", "true");
        } else if (filename.indexOf("work") == 0) {
          hdf.setValue("work", "true");
          hdf.setValue("page.type", "about");
          hdf.setValue("page.category", "work");
        } else if (filename.indexOf("preview") == 0) {
          hdf.setValue("page.type", "preview");
          hdf.setValue("page.category", "preview");
          hdf.setValue("preview", "true");
        } else if (filename.indexOf("auto") == 0) {
          hdf.setValue("auto", "true");
        } else if (filename.indexOf("tv") == 0) {
          hdf.setValue("tv", "true");
        } else if (filename.indexOf("ndk") == 0) {
          hdf.setValue("ndk", "true");
          hdf.setValue("page.type", "ndk");
          if (filename.indexOf("ndk/guides") == 0) {
            hdf.setValue("guide", "true");
          } else if (filename.indexOf("ndk/reference") == 0) {
            hdf.setValue("reference", "true");
          } else if (filename.indexOf("ndk/samples") == 0) {
            hdf.setValue("samples", "true");
            hdf.setValue("samplesDocPage", "true");
          } else if (filename.indexOf("ndk/downloads") == 0) {
            hdf.setValue("downloads", "true");
            fromTemplate = hdf.getValue("page.template", "");
          }
        }
      }
      //set metadata for this file in jd_lists_unified
      PageMetadata.setPageMetadata(docfile, relative, outfile, hdf, Doclava.sTaglist);

      //for devsite builds, remove 'intl/' from output paths for localized files
      if (Doclava.USE_DEVSITE_LOCALE_OUTPUT_PATHS) {
        outfile = outfile.replaceFirst("^intl/", "");
      }

      if (fromTemplate.equals("sdk")) {
        ClearPage.write(hdf, "sdkpage.cs", outfile);
      } else {
        ClearPage.write(hdf, "docpage.cs", outfile);
      }
    }
  } // writePage
}
