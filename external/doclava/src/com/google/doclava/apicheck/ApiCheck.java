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

package com.google.doclava.apicheck;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.io.PrintStream;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.HashSet;
import java.util.Set;
import java.util.Stack;

import com.google.doclava.Errors;
import com.google.doclava.PackageInfo;
import com.google.doclava.Errors.ErrorMessage;
import com.google.doclava.Stubs;

public class ApiCheck {
  // parse out and consume the -whatever command line flags
  private static ArrayList<String[]> parseFlags(ArrayList<String> allArgs) {
    ArrayList<String[]> ret = new ArrayList<String[]>();

    int i;
    for (i = 0; i < allArgs.size(); i++) {
      // flags with one value attached
      String flag = allArgs.get(i);
      if (flag.equals("-error") || flag.equals("-warning") || flag.equals("-hide")
          || flag.equals("-ignoreClass") || flag.equals("-ignorePackage")) {
        String[] arg = new String[2];
        arg[0] = flag;
        arg[1] = allArgs.get(++i);
        ret.add(arg);
      } else {
        // we've consumed all of the -whatever args, so we're done
        break;
      }
    }

    // i now points to the first non-flag arg; strip what came before
    for (; i > 0; i--) {
      allArgs.remove(0);
    }
    return ret;
  }

  public static void main(String[] originalArgs) {
    if (originalArgs.length == 3 && "-convert".equals(originalArgs[0])) {
      System.exit(convertToApi(originalArgs[1], originalArgs[2]));
    } else if (originalArgs.length == 3 && "-convert2xml".equals(originalArgs[0])) {
      System.exit(convertToXml(originalArgs[1], originalArgs[2]));
    } else if (originalArgs.length == 4 && "-new_api".equals(originalArgs[0])) {
      // command syntax: -new_api oldapi.txt newapi.txt diff.xml
      // TODO: Support reading in other options for new_api, such as ignored classes/packages.
      System.exit(newApi(originalArgs[1], originalArgs[2], originalArgs[3]));
    } else {
      ApiCheck acheck = new ApiCheck();
      Report report = acheck.checkApi(originalArgs);

      Errors.printErrors(report.errors());
      System.exit(report.code);
    }
  }
  
  /**
   * Compares two api xml files for consistency.
   */
  public Report checkApi(String[] originalArgs) {
    // translate to an ArrayList<String> for munging
    ArrayList<String> args = new ArrayList<String>(originalArgs.length);
    for (String a : originalArgs) {
      args.add(a);
    }

    // Not having having any classes or packages ignored is the common case.
    // Avoid a hashCode call in a common loop by not passing in a HashSet in this case.
    Set<String> ignoredPackages = null;
    Set<String> ignoredClasses = null;

    ArrayList<String[]> flags = ApiCheck.parseFlags(args);
    for (String[] a : flags) {
      if (a[0].equals("-error") || a[0].equals("-warning") || a[0].equals("-hide")) {
        try {
          int level = -1;
          if (a[0].equals("-error")) {
            level = Errors.ERROR;
          } else if (a[0].equals("-warning")) {
            level = Errors.WARNING;
          } else if (a[0].equals("-hide")) {
            level = Errors.HIDDEN;
          }
          Errors.setErrorLevel(Integer.parseInt(a[1]), level);
        } catch (NumberFormatException e) {
          System.err.println("Bad argument: " + a[0] + " " + a[1]);
          return new Report(2, Errors.getErrors());
        }
      } else if (a[0].equals("-ignoreClass")) {
        if (ignoredClasses == null) {
          ignoredClasses = new HashSet<String>();
        }
        ignoredClasses.add(a[1]);
      } else if (a[0].equals("-ignorePackage")) {
        if (ignoredPackages == null) {
          ignoredPackages = new HashSet<String>();
        }
        ignoredPackages.add(a[1]);
      }
    }

    ApiInfo oldApi;
    ApiInfo newApi;
    ApiInfo oldRemovedApi;
    ApiInfo newRemovedApi;

    // commandline options look like:
    // [other optoins] old_api.txt new_api.txt old_removed_api.txt new_removed_api.txt
    try {
      oldApi = parseApi(args.get(0));
      newApi = parseApi(args.get(1));
      oldRemovedApi = parseApi(args.get(2));
      newRemovedApi = parseApi(args.get(3));
    } catch (ApiParseException e) {
      e.printStackTrace();
      System.err.println("Error parsing API");
      return new Report(1, Errors.getErrors());
    }

    // only run the consistency check if we haven't had XML parse errors
    if (!Errors.hadError) {
      oldApi.isConsistent(newApi, null, ignoredPackages, ignoredClasses);
    }

    if (!Errors.hadError) {
      oldRemovedApi.isConsistent(newRemovedApi, null, ignoredPackages, ignoredClasses);
    }

    return new Report(Errors.hadError ? 1 : 0, Errors.getErrors());
  }

  public static ApiInfo parseApi(String filename) throws ApiParseException {
    InputStream stream = null;
    Throwable textParsingError = null;
    Throwable xmlParsingError = null;
    // try it as our format
    try {
      stream = new FileInputStream(filename);
    } catch (IOException e) {
      throw new ApiParseException("Could not open file for parsing: " + filename, e);
    }
    try {
      return ApiFile.parseApi(filename, stream);
    } catch (ApiParseException exception) {
      textParsingError = exception;
    } finally {
      try {
        stream.close();
      } catch (IOException ignored) {}
    }
    // try it as xml
    try {
      stream = new FileInputStream(filename);
    } catch (IOException e) {
      throw new ApiParseException("Could not open file for parsing: " + filename, e);
    }
    try {
      return XmlApiFile.parseApi(stream);
    } catch (ApiParseException exception) {
      xmlParsingError = exception;
    } finally {
      try {
        stream.close();
      } catch (IOException ignored) {}
    }
    // The file has failed to parse both as XML and as text. Build the string in this order as
    // the message is easier to read with that error at the end.
    throw new ApiParseException(filename +
        " failed to parse as xml: \"" + xmlParsingError.getMessage() +
        "\" and as text: \"" + textParsingError.getMessage() + "\"");
  }

  public ApiInfo parseApi(URL url) throws ApiParseException {
    InputStream stream = null;
    // try it as our format
    try {
      stream = url.openStream();
    } catch (IOException e) {
      throw new ApiParseException("Could not open stream for parsing: " + url, e);
    }
    try {
      return ApiFile.parseApi(url.toString(), stream);
    } catch (ApiParseException ignored) {
    } finally {
      try {
        stream.close();
      } catch (IOException ignored) {}
    }
    // try it as xml
    try {
      stream = url.openStream();
    } catch (IOException e) {
      throw new ApiParseException("Could not open stream for parsing: " + url, e);
    }
    try {
      return XmlApiFile.parseApi(stream);
    } finally {
      try {
        stream.close();
      } catch (IOException ignored) {}
    }
  }

  public class Report {
    private int code;
    private Set<ErrorMessage> errors;
    
    private Report(int code, Set<ErrorMessage> errors) {
      this.code = code;
      this.errors = errors;
    }
    
    public int code() {
      return code;
    }
    
    public Set<ErrorMessage> errors() {
      return errors;
    }
  }

  static int convertToApi(String src, String dst) {
    ApiInfo api;
    try {
      api = parseApi(src);
    } catch (ApiParseException e) {
      e.printStackTrace();
      System.err.println("Error parsing API: " + src);
      return 1;
    }

    PrintStream apiWriter = null;
    try {
      apiWriter = new PrintStream(dst);
    } catch (FileNotFoundException ex) {
      System.err.println("can't open file: " + dst);
    }

    Stubs.writeApi(apiWriter, api.getPackages().values());

    return 0;
  }

  static int convertToXml(String src, String dst) {
    ApiInfo api;
    try {
      api = parseApi(src);
    } catch (ApiParseException e) {
      e.printStackTrace();
      System.err.println("Error parsing API: " + src);
      return 1;
    }

    PrintStream apiWriter = null;
    try {
      apiWriter = new PrintStream(dst);
    } catch (FileNotFoundException ex) {
      System.err.println("can't open file: " + dst);
    }

    Stubs.writeXml(apiWriter, api.getPackages().values());

    return 0;
  }

  /**
   * Generates a "diff": where new API is trimmed down by removing existing methods found in old API
   * @param origApiPath path to old API text file
   * @param newApiPath path to new API text file
   * @param outputPath output XML path for the generated diff
   * @return
   */
  static int newApi(String origApiPath, String newApiPath, String outputPath) {
    ApiInfo origApi, newApi;
    try {
      origApi = parseApi(origApiPath);
    } catch (ApiParseException e) {
      e.printStackTrace();
      System.err.println("Error parsing API: " + origApiPath);
      return 1;
    }
    try {
      newApi = parseApi(newApiPath);
    } catch (ApiParseException e) {
      e.printStackTrace();
      System.err.println("Error parsing API: " + newApiPath);
      return 1;
    }
    List<PackageInfo> pkgInfoDiff = new ArrayList<>();
    if (!origApi.isConsistent(newApi, pkgInfoDiff)) {
      PrintStream apiWriter = null;
      try {
        apiWriter = new PrintStream(outputPath);
      } catch (FileNotFoundException ex) {
        System.err.println("can't open file: " + outputPath);
      }
      Stubs.writeXml(apiWriter, pkgInfoDiff);
    } else {
      System.err.println("No API change detected, not generating diff.");
    }
    return 0;
  }
}
