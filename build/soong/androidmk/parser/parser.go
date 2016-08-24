package parser

import (
	"errors"
	"fmt"
	"io"
	"sort"
	"text/scanner"
)

var errTooManyErrors = errors.New("too many errors")

const maxErrors = 100

type ParseError struct {
	Err error
	Pos scanner.Position
}

func (e *ParseError) Error() string {
	return fmt.Sprintf("%s: %s", e.Pos, e.Err)
}

func (p *parser) Parse() ([]MakeThing, []error) {
	defer func() {
		if r := recover(); r != nil {
			if r == errTooManyErrors {
				return
			}
			panic(r)
		}
	}()

	p.parseLines()
	p.accept(scanner.EOF)
	p.things = append(p.things, p.comments...)
	sort.Sort(byPosition(p.things))

	return p.things, p.errors
}

type parser struct {
	scanner  scanner.Scanner
	tok      rune
	errors   []error
	comments []MakeThing
	things   []MakeThing
}

func NewParser(filename string, r io.Reader) *parser {
	p := &parser{}
	p.scanner.Init(r)
	p.scanner.Error = func(sc *scanner.Scanner, msg string) {
		p.errorf(msg)
	}
	p.scanner.Whitespace = 0
	p.scanner.IsIdentRune = func(ch rune, i int) bool {
		return ch > 0 && ch != ':' && ch != '#' && ch != '=' && ch != '+' && ch != '$' &&
			ch != '\\' && ch != '(' && ch != ')' && ch != '{' && ch != '}' && ch != ';' &&
			ch != '|' && ch != '?' && ch != '\r' && !isWhitespace(ch)
	}
	p.scanner.Mode = scanner.ScanIdents
	p.scanner.Filename = filename
	p.next()
	return p
}

func (p *parser) errorf(format string, args ...interface{}) {
	pos := p.scanner.Position
	if !pos.IsValid() {
		pos = p.scanner.Pos()
	}
	err := &ParseError{
		Err: fmt.Errorf(format, args...),
		Pos: pos,
	}
	p.errors = append(p.errors, err)
	if len(p.errors) >= maxErrors {
		panic(errTooManyErrors)
	}
}

func (p *parser) accept(toks ...rune) bool {
	for _, tok := range toks {
		if p.tok != tok {
			p.errorf("expected %s, found %s", scanner.TokenString(tok),
				scanner.TokenString(p.tok))
			return false
		}
		p.next()
	}
	return true
}

func (p *parser) next() {
	if p.tok != scanner.EOF {
		p.tok = p.scanner.Scan()
		for p.tok == '\r' {
			p.tok = p.scanner.Scan()
		}
	}
	return
}

func (p *parser) parseLines() {
	for {
		p.ignoreWhitespace()

		if p.parseDirective() {
			continue
		}

		ident, _ := p.parseExpression('=', '?', ':', '#', '\n')

		p.ignoreSpaces()

		switch p.tok {
		case '?':
			p.accept('?')
			if p.tok == '=' {
				p.parseAssignment("?=", nil, ident)
			} else {
				p.errorf("expected = after ?")
			}
		case '+':
			p.accept('+')
			if p.tok == '=' {
				p.parseAssignment("+=", nil, ident)
			} else {
				p.errorf("expected = after +")
			}
		case ':':
			p.accept(':')
			switch p.tok {
			case '=':
				p.parseAssignment(":=", nil, ident)
			default:
				p.parseRule(ident)
			}
		case '=':
			p.parseAssignment("=", nil, ident)
		case '#', '\n', scanner.EOF:
			ident.TrimRightSpaces()
			if v, ok := toVariable(ident); ok {
				p.things = append(p.things, v)
			} else if !ident.Empty() {
				p.errorf("expected directive, rule, or assignment after ident " + ident.Dump())
			}
			switch p.tok {
			case scanner.EOF:
				return
			case '\n':
				p.accept('\n')
			case '#':
				p.parseComment()
			}
		default:
			p.errorf("expected assignment or rule definition, found %s\n",
				p.scanner.TokenText())
			return
		}
	}
}

func (p *parser) parseDirective() bool {
	if p.tok != scanner.Ident || !isDirective(p.scanner.TokenText()) {
		return false
	}

	d := p.scanner.TokenText()
	pos := p.scanner.Position
	endPos := pos
	p.accept(scanner.Ident)

	expression := SimpleMakeString("", pos)

	switch d {
	case "endif", "endef", "else":
		// Nothing
	case "define":
		expression = p.parseDefine()
	default:
		p.ignoreSpaces()
		expression, endPos = p.parseExpression()
	}

	p.things = append(p.things, Directive{
		makeThing: makeThing{
			pos:    pos,
			endPos: endPos,
		},
		Name: d,
		Args: expression,
	})
	return true
}

func (p *parser) parseDefine() *MakeString {
	value := SimpleMakeString("", p.scanner.Position)

loop:
	for {
		switch p.tok {
		case scanner.Ident:
			if p.scanner.TokenText() == "endef" {
				p.accept(scanner.Ident)
				break loop
			}
			value.appendString(p.scanner.TokenText())
			p.accept(scanner.Ident)
		case '\\':
			p.parseEscape()
			switch p.tok {
			case '\n':
				value.appendString(" ")
			case scanner.EOF:
				p.errorf("expected escaped character, found %s",
					scanner.TokenString(p.tok))
				break loop
			default:
				value.appendString(`\` + string(p.tok))
			}
			p.accept(p.tok)
		//TODO: handle variables inside defines?  result depends if
		//define is used in make or rule context
		//case '$':
		//	variable := p.parseVariable()
		//	value.appendVariable(variable)
		case scanner.EOF:
			p.errorf("unexpected EOF while looking for endef")
			break loop
		default:
			value.appendString(p.scanner.TokenText())
			p.accept(p.tok)
		}
	}

	return value
}

func (p *parser) parseEscape() {
	p.scanner.Mode = 0
	p.accept('\\')
	p.scanner.Mode = scanner.ScanIdents
}

func (p *parser) parseExpression(end ...rune) (*MakeString, scanner.Position) {
	value := SimpleMakeString("", p.scanner.Position)

	endParen := false
	for _, r := range end {
		if r == ')' {
			endParen = true
		}
	}
	parens := 0

	endPos := p.scanner.Position

loop:
	for {
		if endParen && parens > 0 && p.tok == ')' {
			parens--
			value.appendString(")")
			endPos = p.scanner.Position
			p.accept(')')
			continue
		}

		for _, r := range end {
			if p.tok == r {
				break loop
			}
		}

		switch p.tok {
		case '\n':
			break loop
		case scanner.Ident:
			value.appendString(p.scanner.TokenText())
			endPos = p.scanner.Position
			p.accept(scanner.Ident)
		case '\\':
			p.parseEscape()
			switch p.tok {
			case '\n':
				value.appendString(" ")
			case scanner.EOF:
				p.errorf("expected escaped character, found %s",
					scanner.TokenString(p.tok))
				return value, endPos
			default:
				value.appendString(`\` + string(p.tok))
			}
			endPos = p.scanner.Position
			p.accept(p.tok)
		case '#':
			p.parseComment()
			break loop
		case '$':
			var variable Variable
			variable, endPos = p.parseVariable()
			value.appendVariable(variable)
		case scanner.EOF:
			break loop
		case '(':
			if endParen {
				parens++
			}
			value.appendString("(")
			endPos = p.scanner.Position
			p.accept('(')
		default:
			value.appendString(p.scanner.TokenText())
			endPos = p.scanner.Position
			p.accept(p.tok)
		}
	}

	if parens > 0 {
		p.errorf("expected closing paren %s", value.Dump())
	}
	return value, endPos
}

func (p *parser) parseVariable() (Variable, scanner.Position) {
	pos := p.scanner.Position
	endPos := pos
	p.accept('$')
	var name *MakeString
	switch p.tok {
	case '(':
		return p.parseBracketedVariable('(', ')', pos)
	case '{':
		return p.parseBracketedVariable('{', '}', pos)
	case '$':
		name = SimpleMakeString("__builtin_dollar", scanner.Position{})
	case scanner.EOF:
		p.errorf("expected variable name, found %s",
			scanner.TokenString(p.tok))
	default:
		name, endPos = p.parseExpression(variableNameEndRunes...)
	}

	return p.nameToVariable(name, pos, endPos), endPos
}

func (p *parser) parseBracketedVariable(start, end rune, pos scanner.Position) (Variable, scanner.Position) {
	p.accept(start)
	name, endPos := p.parseExpression(end)
	p.accept(end)
	return p.nameToVariable(name, pos, endPos), endPos
}

func (p *parser) nameToVariable(name *MakeString, pos, endPos scanner.Position) Variable {
	return Variable{
		makeThing: makeThing{
			pos:    pos,
			endPos: endPos,
		},
		Name: name,
	}
}

func (p *parser) parseRule(target *MakeString) {
	prerequisites, newLine := p.parseRulePrerequisites(target)

	recipe := ""
	endPos := p.scanner.Position
loop:
	for {
		if newLine {
			if p.tok == '\t' {
				endPos = p.scanner.Position
				p.accept('\t')
				newLine = false
				continue loop
			} else if p.parseDirective() {
				newLine = false
				continue
			} else {
				break loop
			}
		}

		newLine = false
		switch p.tok {
		case '\\':
			p.parseEscape()
			recipe += string(p.tok)
			endPos = p.scanner.Position
			p.accept(p.tok)
		case '\n':
			newLine = true
			recipe += "\n"
			endPos = p.scanner.Position
			p.accept('\n')
		case scanner.EOF:
			break loop
		default:
			recipe += p.scanner.TokenText()
			endPos = p.scanner.Position
			p.accept(p.tok)
		}
	}

	if prerequisites != nil {
		p.things = append(p.things, Rule{
			makeThing: makeThing{
				pos:    target.Pos,
				endPos: endPos,
			},
			Target:        target,
			Prerequisites: prerequisites,
			Recipe:        recipe,
		})
	}
}

func (p *parser) parseRulePrerequisites(target *MakeString) (*MakeString, bool) {
	newLine := false

	p.ignoreSpaces()

	prerequisites, _ := p.parseExpression('#', '\n', ';', ':', '=')

	switch p.tok {
	case '\n':
		p.accept('\n')
		newLine = true
	case '#':
		p.parseComment()
		newLine = true
	case ';':
		p.accept(';')
	case ':':
		p.accept(':')
		if p.tok == '=' {
			p.parseAssignment(":=", target, prerequisites)
			return nil, true
		} else {
			more, _ := p.parseExpression('#', '\n', ';')
			prerequisites.appendMakeString(more)
		}
	case '=':
		p.parseAssignment("=", target, prerequisites)
		return nil, true
	default:
		p.errorf("unexpected token %s after rule prerequisites", scanner.TokenString(p.tok))
	}

	return prerequisites, newLine
}

func (p *parser) parseComment() {
	pos := p.scanner.Position
	p.accept('#')
	comment := ""
	endPos := pos
loop:
	for {
		switch p.tok {
		case '\\':
			p.parseEscape()
			if p.tok == '\n' {
				comment += "\n"
			} else {
				comment += "\\" + p.scanner.TokenText()
			}
			endPos = p.scanner.Position
			p.accept(p.tok)
		case '\n':
			endPos = p.scanner.Position
			p.accept('\n')
			break loop
		case scanner.EOF:
			break loop
		default:
			comment += p.scanner.TokenText()
			endPos = p.scanner.Position
			p.accept(p.tok)
		}
	}

	p.comments = append(p.comments, Comment{
		makeThing: makeThing{
			pos:    pos,
			endPos: endPos,
		},
		Comment: comment,
	})
}

func (p *parser) parseAssignment(t string, target *MakeString, ident *MakeString) {
	// The value of an assignment is everything including and after the first
	// non-whitespace character after the = until the end of the logical line,
	// which may included escaped newlines
	p.accept('=')
	value, endPos := p.parseExpression()
	value.TrimLeftSpaces()
	if ident.EndsWith('+') && t == "=" {
		ident.TrimRightOne()
		t = "+="
	}

	ident.TrimRightSpaces()

	p.things = append(p.things, Assignment{
		makeThing: makeThing{
			pos:    ident.Pos,
			endPos: endPos,
		},
		Name:   ident,
		Value:  value,
		Target: target,
		Type:   t,
	})
}

type androidMkModule struct {
	assignments map[string]string
}

type androidMkFile struct {
	assignments map[string]string
	modules     []androidMkModule
	includes    []string
}

var directives = [...]string{
	"define",
	"else",
	"endef",
	"endif",
	"ifdef",
	"ifeq",
	"ifndef",
	"ifneq",
	"include",
	"-include",
}

var functions = [...]string{
	"abspath",
	"addprefix",
	"addsuffix",
	"basename",
	"dir",
	"notdir",
	"subst",
	"suffix",
	"filter",
	"filter-out",
	"findstring",
	"firstword",
	"flavor",
	"join",
	"lastword",
	"patsubst",
	"realpath",
	"shell",
	"sort",
	"strip",
	"wildcard",
	"word",
	"wordlist",
	"words",
	"origin",
	"foreach",
	"call",
	"info",
	"error",
	"warning",
	"if",
	"or",
	"and",
	"value",
	"eval",
	"file",
}

func init() {
	sort.Strings(directives[:])
	sort.Strings(functions[:])
}

func isDirective(s string) bool {
	for _, d := range directives {
		if s == d {
			return true
		} else if s < d {
			return false
		}
	}
	return false
}

func isFunctionName(s string) bool {
	for _, f := range functions {
		if s == f {
			return true
		} else if s < f {
			return false
		}
	}
	return false
}

func isWhitespace(ch rune) bool {
	return ch == ' ' || ch == '\t' || ch == '\n'
}

func isValidVariableRune(ch rune) bool {
	return ch != scanner.Ident && ch != ':' && ch != '=' && ch != '#'
}

var whitespaceRunes = []rune{' ', '\t', '\n'}
var variableNameEndRunes = append([]rune{':', '=', '#', ')', '}'}, whitespaceRunes...)

func (p *parser) ignoreSpaces() int {
	skipped := 0
	for p.tok == ' ' || p.tok == '\t' {
		p.accept(p.tok)
		skipped++
	}
	return skipped
}

func (p *parser) ignoreWhitespace() {
	for isWhitespace(p.tok) {
		p.accept(p.tok)
	}
}
