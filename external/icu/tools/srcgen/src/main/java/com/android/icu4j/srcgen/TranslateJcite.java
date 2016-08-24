/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.android.icu4j.srcgen;

import com.google.common.base.Splitter;
import com.google.common.collect.Sets;
import com.google.currysrc.api.process.Reporter;
import com.google.currysrc.api.process.ast.AstNodes;
import com.google.currysrc.api.process.ast.BodyDeclarationLocator;
import com.google.currysrc.processors.BaseModifyCommentScanner;
import com.google.currysrc.processors.BaseTagElementNodeScanner;

import org.eclipse.jdt.core.dom.AST;
import org.eclipse.jdt.core.dom.ASTNode;
import org.eclipse.jdt.core.dom.BodyDeclaration;
import org.eclipse.jdt.core.dom.Comment;
import org.eclipse.jdt.core.dom.IDocElement;
import org.eclipse.jdt.core.dom.LineComment;
import org.eclipse.jdt.core.dom.TagElement;
import org.eclipse.jdt.core.dom.rewrite.ASTRewrite;

import java.util.List;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import static com.google.currysrc.api.process.ast.BodyDeclarationLocators.findDeclarationNode;
import static com.google.currysrc.api.process.ast.BodyDeclarationLocators.matchesAny;

/**
 * Classes for handling {@literal @}.jcite tags used by ICU.
 */
public class TranslateJcite {

  /** The string used to escape a jcite tag. */
  public static final String ESCAPED_JCITE_TAG = "{@literal @}.jcite";

  private TranslateJcite() {}

  /**
   * Translate JCite "target" tags in comments like
   * {@code // ---fooBar}
   * to
   * {@code // BEGIN_INCLUDE(fooBar)} and {@code // END_INCLUDE(fooBar)}.
   */
  public static class BeginEndTagsHandler extends BaseModifyCommentScanner {

    private static final Pattern JCITE_TAG_PATTERN = Pattern.compile("//\\s+---(\\S*)\\s*");
    private final Set<String> startedJciteTags = Sets.newHashSet();
    private final Set<String> endedJciteTags = Sets.newHashSet();

    @Override
    protected String processComment(Reporter reporter, Comment commentNode, String commentText) {
      if (!(commentNode instanceof LineComment)) {
        return null;
      }
      Matcher matcher = JCITE_TAG_PATTERN.matcher(commentText);
      if (!matcher.matches()) {
        return null;
      }

      String jciteTag = matcher.group(1);

      // Comments are passed in reverse order.

      // jcite allows the same tags to be used multiple times. As of ICU56, ICU has up to 2 blocks
      // per file.
      // @sample does not deal with multiple BEGIN_INCLUDE / END_INCLUDE tags. As a hack we only
      // deal with the last instance with a given tag in the file. The first is usually imports and
      // we ignore them.
      if (startedJciteTags.contains(jciteTag)) {
        // Just record the fact in the output file that we've been here with text that will be easy
        // to find (in order to find this code).
        return "// IGNORED_INCLUDE(" + jciteTag + ")";
      }

      if (endedJciteTags.contains(jciteTag)) {
        startedJciteTags.add(jciteTag);
        return "// BEGIN_INCLUDE(" + jciteTag + ")";
      } else {
        endedJciteTags.add(jciteTag);
        return "// END_INCLUDE(" + jciteTag + ")";
      }
    }
  }

  /**
   * Translates [{@literal@}.jcite [classname]:---[tag name]]
   * to
   * [{@literal@}sample [source file name] [tag]]
   * if the declaration it is associated with appears in a whitelist.
   */
  public static class InclusionHandler extends BaseTagElementNodeScanner {

    private final String sampleSrcDir;

    private final List<BodyDeclarationLocator> whitelist;

    public InclusionHandler(String sampleSrcDir, List<BodyDeclarationLocator> whitelist) {
      this.sampleSrcDir = sampleSrcDir;
      this.whitelist = whitelist;
    }

    @Override
    protected boolean visitTagElement(Reporter reporter, ASTRewrite rewrite, TagElement tagNode) {
      String tagName = tagNode.getTagName();
      if (tagName == null || !tagName.equalsIgnoreCase("@.jcite")) {
        return true;
      }

      // Determine if this is one of the whitelisted tags and create the appropriate replacement.
      BodyDeclaration declarationNode = findDeclarationNode(tagNode);
      if (declarationNode == null) {
        throw new AssertionError("Unable to find declaration for " + tagNode);
      }
      boolean matchesWhitelist = matchesAny(whitelist, declarationNode);
      TagElement replacementTagNode;
      if (matchesWhitelist) {
        replacementTagNode = createSampleTagElement(tagNode);
      } else {
        replacementTagNode = createEscapedJciteTagElement(tagNode);
      }

      // Hack notice: Replacing a nested TagElement tends to mess up the nesting (e.g. we lose
      // enclosing {}'s). Guess: It's because the replacementTagNode is not considered "nested"
      // because it doesn't have a TagElement parent until it is in the AST.
      // Workaround below: Wrap it in another TagElement with no name.
      TagElement fakeWrapper = tagNode.getAST().newTagElement();
      fakeWrapper.fragments().add(replacementTagNode);

      rewrite.replace(tagNode, fakeWrapper, null /* editGroup */);
      return false;
    }

    private TagElement createSampleTagElement(TagElement tagNode) {
      List<IDocElement> fragments = tagNode.fragments();
      if (fragments.size() != 1) {
        throw new AssertionError("Badly formed .jcite tag: one fragment expected");
      }
      String fragmentText = fragments.get(0).toString().trim();
      int colonIndex = fragmentText.indexOf(':');
      if (colonIndex == -1) {
        throw new AssertionError("Badly formed .jcite tag: expected ':'");
      }
      List<String> jciteElements = Splitter.on(":").splitToList(fragmentText);
      if (jciteElements.size() != 2) {
        throw new AssertionError("Badly formed .jcite tag: expected 2 components");
      }

      String className = jciteElements.get(0);
      String snippetLocator = jciteElements.get(1);

      String fileName = sampleSrcDir + '/' + className.replace('.', '/') + ".java";

      String snippetLocatorPrefix = "---";
      if (!snippetLocator.startsWith(snippetLocatorPrefix)) {
        throw new AssertionError("Badly formed .jcite tag: expected --- on snippetLocator");
      }
      // See the TranslateJciteBeginEndTags transformer.
      String newTag = snippetLocator.substring(snippetLocatorPrefix.length());
      // Remove any trailing whitespace.
      newTag = newTag.trim();

      AST ast = tagNode.getAST();
      return AstNodes.createTextTagElement(ast, "@sample " + fileName + " " + newTag);
    }

    private TagElement createEscapedJciteTagElement(TagElement tagNode) {
      // Note: This doesn't quite work properly: it introduces an extra space between the escaped
      // name and the rest of the tag. e.g. {@literal @}.jcite  foo.bar.....
      AST ast = tagNode.getAST();
      TagElement replacement = ast.newTagElement();
      replacement.fragments().add(AstNodes.createTextElement(ast, ESCAPED_JCITE_TAG));
      replacement.fragments().addAll(ASTNode.copySubtrees(ast, tagNode.fragments()));
      return replacement;
    }
  }
}
