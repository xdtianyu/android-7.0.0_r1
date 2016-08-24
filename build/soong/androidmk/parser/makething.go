package parser

import (
	"text/scanner"
)

type MakeThing interface {
	AsAssignment() (Assignment, bool)
	AsComment() (Comment, bool)
	AsDirective() (Directive, bool)
	AsRule() (Rule, bool)
	AsVariable() (Variable, bool)
	Dump() string
	Pos() scanner.Position
	EndPos() scanner.Position
}

type Assignment struct {
	makeThing
	Name   *MakeString
	Value  *MakeString
	Target *MakeString
	Type   string
}

type Comment struct {
	makeThing
	Comment string
}

type Directive struct {
	makeThing
	Name string
	Args *MakeString
}

type Rule struct {
	makeThing
	Target        *MakeString
	Prerequisites *MakeString
	Recipe        string
}

type Variable struct {
	makeThing
	Name *MakeString
}

type makeThing struct {
	pos    scanner.Position
	endPos scanner.Position
}

func (m makeThing) Pos() scanner.Position {
	return m.pos
}

func (m makeThing) EndPos() scanner.Position {
	return m.endPos
}

func (makeThing) AsAssignment() (a Assignment, ok bool) {
	return
}

func (a Assignment) AsAssignment() (Assignment, bool) {
	return a, true
}

func (a Assignment) Dump() string {
	target := ""
	if a.Target != nil {
		target = a.Target.Dump() + ": "
	}
	return target + a.Name.Dump() + a.Type + a.Value.Dump()
}

func (makeThing) AsComment() (c Comment, ok bool) {
	return
}

func (c Comment) AsComment() (Comment, bool) {
	return c, true
}

func (c Comment) Dump() string {
	return "#" + c.Comment
}

func (makeThing) AsDirective() (d Directive, ok bool) {
	return
}

func (d Directive) AsDirective() (Directive, bool) {
	return d, true
}

func (d Directive) Dump() string {
	return d.Name + " " + d.Args.Dump()
}

func (makeThing) AsRule() (r Rule, ok bool) {
	return
}

func (r Rule) AsRule() (Rule, bool) {
	return r, true
}

func (r Rule) Dump() string {
	recipe := ""
	if r.Recipe != "" {
		recipe = "\n" + r.Recipe
	}
	return "rule:       " + r.Target.Dump() + ": " + r.Prerequisites.Dump() + recipe
}

func (makeThing) AsVariable() (v Variable, ok bool) {
	return
}

func (v Variable) AsVariable() (Variable, bool) {
	return v, true
}

func (v Variable) Dump() string {
	return "$(" + v.Name.Dump() + ")"
}

type byPosition []MakeThing

func (s byPosition) Len() int {
	return len(s)
}

func (s byPosition) Swap(i, j int) {
	s[i], s[j] = s[j], s[i]
}

func (s byPosition) Less(i, j int) bool {
	return s[i].Pos().Offset < s[j].Pos().Offset
}
