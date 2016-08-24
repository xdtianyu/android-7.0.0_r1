// This file is part of TagSoup and is Copyright 2002-2008 by John Cowan.
//
// TagSoup is licensed under the Apache License,
// Version 2.0.  You may obtain a copy of this license at
// http://www.apache.org/licenses/LICENSE-2.0 .  You may also have
// additional legal rights not granted by this license.
//
// TagSoup is distributed in the hope that it will be useful, but
// unless required by applicable law or agreed to in writing, TagSoup
// is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
// OF ANY KIND, either express or implied; not even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// 
// 
package org.ccil.cowan.tagsoup;
import java.io.*;
import org.xml.sax.SAXException;
import org.xml.sax.Locator;

/**
This class implements a table-driven scanner for HTML, allowing for lots of
defects.  It implements the Scanner interface, which accepts a Reader
object to fetch characters from and a ScanHandler object to report lexical
events to.
*/

public class HTMLScanner implements Scanner, Locator {

	// Start of state table
	@@STATE_TABLE@@
	// End of state table

	private String thePublicid;			// Locator state
	private String theSystemid;
	private int theLastLine;
	private int theLastColumn;
	private int theCurrentLine;
	private int theCurrentColumn;

	int theState;					// Current state
	int theNextState;				// Next state
	char[] theOutputBuffer = new char[200];	// Output buffer
	int theSize;					// Current buffer size
	int[] theWinMap = {				// Windows chars map
		0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
		0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
		0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
		0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0x0178};

	/**
	 * Index into the state table for [state][input character - 2].
	 * The state table consists of 4-entry runs on the form
	 * { current state, input character, action, next state }.
	 * We precompute the index into the state table for all possible
	 * { current state, input character } and store the result in
	 * the statetableIndex array. Since only some input characters
	 * are present in the state table, we only do the computation for
	 * characters 0 to the highest character value in the state table.
	 * An input character of -2 is used to cover all other characters
	 * as -2 is guaranteed not to match any input character entry
	 * in the state table.
	 *
	 * <p>When doing lookups, the input character should first be tested
	 * to be in the range [-1 (inclusive), statetableIndexMaxChar (exclusive)].
	 * if it isn't use -2 as the input character.
	 * 
	 * <p>Finally, add 2 to the input character to cover for the fact that
	 * Java doesn't support negative array indexes. Then look up
	 * the value in the statetableIndex. If the value is -1, then
	 * no action or next state was found for the { state, input } that
	 * you had. If it isn't -1, then action = statetable[value + 2] and
	 * next state = statetable[value + 3]. That is, the value points
	 * to the start of the answer 4-tuple in the statetable.
	 */
	static short[][] statetableIndex;
	/**
	 * The highest character value seen in the statetable.
	 * See the doc comment for statetableIndex to see how this
	 * is used.
	 */
	static int statetableIndexMaxChar;
	static {
		int maxState = -1;
		int maxChar = -1;
		for (int i = 0; i < statetable.length; i += 4) {
			if (statetable[i] > maxState) {
				maxState = statetable[i];
				}
			if (statetable[i + 1] > maxChar) {
				maxChar = statetable[i + 1];
				}
			}
		statetableIndexMaxChar = maxChar + 1;

		statetableIndex = new short[maxState + 1][maxChar + 3];
		for (int theState = 0; theState <= maxState; ++theState) {
			for (int ch = -2; ch <= maxChar; ++ch) {
				int hit = -1;
				int action = 0;
				for (int i = 0; i < statetable.length; i += 4) {
					if (theState != statetable[i]) {
						if (action != 0) break;
						continue;
						}
					if (statetable[i+1] == 0) {
						hit = i;
						action = statetable[i+2];
						}
					else if (statetable[i+1] == ch) {
						hit = i;
						action = statetable[i+2];
						break;
						}
					}
				statetableIndex[theState][ch + 2] = (short) hit;
				}
			}
		}

	// Compensate for bug in PushbackReader that allows
	// pushing back EOF.
	private void unread(PushbackReader r, int c) throws IOException {
		if (c != -1) r.unread(c);
		}

	// Locator implementation

	public int getLineNumber() {
		return theLastLine;
		}
	public int getColumnNumber() {
		return theLastColumn;
		}
	public String getPublicId() {
		return thePublicid;
		}
	public String getSystemId() {
		return theSystemid;
		}


	// Scanner implementation

	/**
	Reset document locator, supplying systemid and publicid.
	@param systemid System id
	@param publicid Public id
	*/

	public void resetDocumentLocator(String publicid, String systemid) {
		thePublicid = publicid;
		theSystemid = systemid;
		theLastLine = theLastColumn = theCurrentLine = theCurrentColumn = 0;
		}

	/**
	Scan HTML source, reporting lexical events.
	@param r0 Reader that provides characters
	@param h ScanHandler that accepts lexical events.
	*/

	public void scan(Reader r0, ScanHandler h) throws IOException, SAXException {
		theState = S_PCDATA;
		PushbackReader r;
		if (r0 instanceof BufferedReader) {
			r = new PushbackReader(r0, 5);
			}
		else {
			r = new PushbackReader(new BufferedReader(r0), 5);
			}

		int firstChar = r.read();	// Remove any leading BOM
		if (firstChar != '\uFEFF') unread(r, firstChar);

		while (theState != S_DONE) {
			int ch = r.read();

			// Process control characters
			if (ch >= 0x80 && ch <= 0x9F) ch = theWinMap[ch-0x80];

			if (ch == '\r') {
				ch = r.read();		// expect LF next
				if (ch != '\n') {
					unread(r, ch);	// nope
					ch = '\n';
					}
				}

			if (ch == '\n') {
				theCurrentLine++;
				theCurrentColumn = 0;
				}
			else {
				theCurrentColumn++;
				}

			if (!(ch >= 0x20 || ch == '\n' || ch == '\t' || ch == -1)) continue;

			// Search state table
			int adjCh = (ch >= -1 && ch < statetableIndexMaxChar) ? ch : -2;
			int statetableRow = statetableIndex[theState][adjCh + 2];
			int action = 0;
			if (statetableRow != -1) {
				action = statetable[statetableRow + 2];
				theNextState = statetable[statetableRow + 3];
				}

//			System.err.println("In " + debug_statenames[theState] + " got " + nicechar(ch) + " doing " + debug_actionnames[action] + " then " + debug_statenames[theNextState]);
			switch (action) {
			case 0:
				throw new Error(
					"HTMLScanner can't cope with " + Integer.toString(ch) + " in state " +
					Integer.toString(theState));
			case A_ADUP:
				h.adup(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_ADUP_SAVE:
				h.adup(theOutputBuffer, 0, theSize);
				theSize = 0;
				save(ch, h);
				break;
			case A_ADUP_STAGC:
				h.adup(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.stagc(theOutputBuffer, 0, theSize);
				break;
			case A_ANAME:
				h.aname(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_ANAME_ADUP:
				h.aname(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.adup(theOutputBuffer, 0, theSize);
				break;
			case A_ANAME_ADUP_STAGC:
				h.aname(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.adup(theOutputBuffer, 0, theSize);
				h.stagc(theOutputBuffer, 0, theSize);
				break;
			case A_AVAL:
				h.aval(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_AVAL_STAGC:
				h.aval(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.stagc(theOutputBuffer, 0, theSize);
				break;
			case A_CDATA:
				mark();
				// suppress the final "]]" in the buffer
				if (theSize > 1) theSize -= 2;
				h.pcdata(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_ENTITY_START:
				h.pcdata(theOutputBuffer, 0, theSize);
				theSize = 0;
				save(ch, h);
				break;
			case A_ENTITY:
				mark();
				char ch1 = (char)ch;
//				System.out.println("Got " + ch1 + " in state " + ((theState == S_ENT) ? "S_ENT" : ((theState == S_NCR) ? "S_NCR" : "UNK")));
				if (theState == S_ENT && ch1 == '#') {
					theNextState = S_NCR;
					save(ch, h);
					break;
					}
				else if (theState == S_NCR && (ch1 == 'x' || ch1 == 'X')) {
					theNextState = S_XNCR;
					save(ch, h);
					break;
					}
				else if (theState == S_ENT && Character.isLetterOrDigit(ch1)) {
					save(ch, h);
					break;
					}
				else if (theState == S_NCR && Character.isDigit(ch1)) {
					save(ch, h);
					break;
					}
				else if (theState == S_XNCR && (Character.isDigit(ch1) || "abcdefABCDEF".indexOf(ch1) != -1)) {
					save(ch, h);
					break;
					}

				// The whole entity reference has been collected
//				System.err.println("%%" + new String(theOutputBuffer, 0, theSize));
				h.entity(theOutputBuffer, 1, theSize - 1);
				int ent = h.getEntity();
//				System.err.println("%% value = " + ent);
				if (ent != 0) {
					theSize = 0;
					if (ent >= 0x80 && ent <= 0x9F) {
						ent = theWinMap[ent-0x80];
						}
					if (ent < 0x20) {
						// Control becomes space
						ent = 0x20;
						}
					else if (ent >= 0xD800 && ent <= 0xDFFF) {
						// Surrogates get dropped
						ent = 0;
						}
					else if (ent <= 0xFFFF) {
						// BMP character
						save(ent, h);
						}
					else {
						// Astral converted to two surrogates
						ent -= 0x10000;
						save((ent>>10) + 0xD800, h);
						save((ent&0x3FF) + 0xDC00, h);
						}
					if (ch != ';') {
						unread(r, ch);
						theCurrentColumn--;
						}
					}
				else {
					unread(r, ch);
					theCurrentColumn--;
					}
				theNextState = S_PCDATA;
				break;
			case A_ETAG:
				h.etag(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_DECL:
				h.decl(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_GI:
				h.gi(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_GI_STAGC:
				h.gi(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.stagc(theOutputBuffer, 0, theSize);
				break;
			case A_LT:
				mark();
				save('<', h);
				save(ch, h);
				break;
			case A_LT_PCDATA:
				mark();
				save('<', h);
				h.pcdata(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_PCDATA:
				mark();
				h.pcdata(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_CMNT:
				mark();
				h.cmnt(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_MINUS3:
				save('-', h);
				save(' ', h);
				break;
			case A_MINUS2:
				save('-', h);
				save(' ', h);
				// fall through into A_MINUS
			case A_MINUS:
				save('-', h);
				save(ch, h);
				break;
			case A_PI:
				mark();
				h.pi(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_PITARGET:
				h.pitarget(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_PITARGET_PI:
				h.pitarget(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.pi(theOutputBuffer, 0, theSize);
				break;
			case A_SAVE:
				save(ch, h);
				break;
			case A_SKIP:
				break;
			case A_SP:
				save(' ', h);
				break;
			case A_STAGC:
				h.stagc(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			case A_EMPTYTAG:
				mark();
//				System.err.println("%%% Empty tag seen");
				if (theSize > 0) h.gi(theOutputBuffer, 0, theSize);
				theSize = 0;
				h.stage(theOutputBuffer, 0, theSize);
				break;
			case A_UNGET:
				unread(r, ch);
				theCurrentColumn--;
				break;
			case A_UNSAVE_PCDATA:
				if (theSize > 0) theSize--;
				h.pcdata(theOutputBuffer, 0, theSize);
				theSize = 0;
				break;
			default:
				throw new Error("Can't process state " + action);
				}
			theState = theNextState;
			}
		h.eof(theOutputBuffer, 0, 0);
		}

	/**
	* Mark the current scan position as a "point of interest" - start of a tag,
	* cdata, processing instruction etc.
	*/

	private void mark() {
		theLastColumn = theCurrentColumn;
		theLastLine = theCurrentLine;
		}

	/**
	A callback for the ScanHandler that allows it to force
	the lexer state to CDATA content (no markup is recognized except
	the end of element.
	*/

	public void startCDATA() { theNextState = S_CDATA; }

	private void save(int ch, ScanHandler h) throws IOException, SAXException {
		if (theSize >= theOutputBuffer.length - 20) {
			if (theState == S_PCDATA || theState == S_CDATA) {
				// Return a buffer-sized chunk of PCDATA
				h.pcdata(theOutputBuffer, 0, theSize);
				theSize = 0;
				}
			else {
				// Grow the buffer size
				char[] newOutputBuffer = new char[theOutputBuffer.length * 2];
				System.arraycopy(theOutputBuffer, 0, newOutputBuffer, 0, theSize+1);
				theOutputBuffer = newOutputBuffer;
				}
			}
		theOutputBuffer[theSize++] = (char)ch;
		}

	/**
	Test procedure.  Reads HTML from the standard input and writes
	PYX to the standard output.
	*/

	public static void main(String[] argv) throws IOException, SAXException {
		Scanner s = new HTMLScanner();
		Reader r = new InputStreamReader(System.in, "UTF-8");
		Writer w = new OutputStreamWriter(System.out, "UTF-8");
		PYXWriter pw = new PYXWriter(w);
		s.scan(r, pw);
		w.close();
		}


	private static String nicechar(int in) {
		if (in == '\n') return "\\n";
		if (in < 32) return "0x"+Integer.toHexString(in);
		return "'"+((char)in)+"'";
		}

	}
