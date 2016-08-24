package parser

import (
	"strings"
	"testing"
	"text/scanner"
)

var splitNTestCases = []struct {
	in       *MakeString
	expected []*MakeString
	sep      string
	n        int
}{
	{
		in: &MakeString{
			Strings: []string{
				"a b c",
				"d e f",
				" h i j",
			},
			Variables: []Variable{
				Variable{Name: SimpleMakeString("var1", scanner.Position{})},
				Variable{Name: SimpleMakeString("var2", scanner.Position{})},
			},
		},
		sep: " ",
		n:   -1,
		expected: []*MakeString{
			SimpleMakeString("a", scanner.Position{}),
			SimpleMakeString("b", scanner.Position{}),
			&MakeString{
				Strings: []string{"c", "d"},
				Variables: []Variable{
					Variable{Name: SimpleMakeString("var1", scanner.Position{})},
				},
			},
			SimpleMakeString("e", scanner.Position{}),
			&MakeString{
				Strings: []string{"f", ""},
				Variables: []Variable{
					Variable{Name: SimpleMakeString("var2", scanner.Position{})},
				},
			},
			SimpleMakeString("h", scanner.Position{}),
			SimpleMakeString("i", scanner.Position{}),
			SimpleMakeString("j", scanner.Position{}),
		},
	},
	{
		in: &MakeString{
			Strings: []string{
				"a b c",
				"d e f",
				" h i j",
			},
			Variables: []Variable{
				Variable{Name: SimpleMakeString("var1", scanner.Position{})},
				Variable{Name: SimpleMakeString("var2", scanner.Position{})},
			},
		},
		sep: " ",
		n:   3,
		expected: []*MakeString{
			SimpleMakeString("a", scanner.Position{}),
			SimpleMakeString("b", scanner.Position{}),
			&MakeString{
				Strings: []string{"c", "d e f", " h i j"},
				Variables: []Variable{
					Variable{Name: SimpleMakeString("var1", scanner.Position{})},
					Variable{Name: SimpleMakeString("var2", scanner.Position{})},
				},
			},
		},
	},
}

func TestMakeStringSplitN(t *testing.T) {
	for _, test := range splitNTestCases {
		got := test.in.SplitN(test.sep, test.n)
		gotString := dumpArray(got)
		expectedString := dumpArray(test.expected)
		if gotString != expectedString {
			t.Errorf("expected:\n%s\ngot:\n%s", expectedString, gotString)
		}
	}
}

func dumpArray(a []*MakeString) string {
	ret := make([]string, len(a))

	for i, s := range a {
		ret[i] = s.Dump()
	}

	return strings.Join(ret, "|||")
}
