package main

import (
	"fmt"
	"strings"

	mkparser "android/soong/androidmk/parser"

	bpparser "github.com/google/blueprint/parser"
)

func stringToStringValue(s string) *bpparser.Value {
	return &bpparser.Value{
		Type:        bpparser.String,
		StringValue: s,
	}
}

func addValues(val1, val2 *bpparser.Value) (*bpparser.Value, error) {
	if val1 == nil {
		return val2, nil
	}

	if val1.Type == bpparser.String && val2.Type == bpparser.List {
		val1 = &bpparser.Value{
			Type:      bpparser.List,
			ListValue: []bpparser.Value{*val1},
		}
	} else if val2.Type == bpparser.String && val1.Type == bpparser.List {
		val2 = &bpparser.Value{
			Type:      bpparser.List,
			ListValue: []bpparser.Value{*val1},
		}
	} else if val1.Type != val2.Type {
		return nil, fmt.Errorf("cannot add mismatched types")
	}

	return &bpparser.Value{
		Type: val1.Type,
		Expression: &bpparser.Expression{
			Operator: '+',
			Args:     [2]bpparser.Value{*val1, *val2},
		},
	}, nil
}

func makeToStringExpression(ms *mkparser.MakeString, scope mkparser.Scope) (*bpparser.Value, error) {
	var val *bpparser.Value
	var err error

	if ms.Strings[0] != "" {
		val = stringToStringValue(ms.Strings[0])
	}

	for i, s := range ms.Strings[1:] {
		if ret, ok := ms.Variables[i].EvalFunction(scope); ok {
			val, err = addValues(val, stringToStringValue(ret))
		} else {
			name := ms.Variables[i].Name
			if !name.Const() {
				return nil, fmt.Errorf("Unsupported non-const variable name %s", name.Dump())
			}
			tmp := &bpparser.Value{
				Type:     bpparser.String,
				Variable: name.Value(nil),
			}

			val, err = addValues(val, tmp)
			if err != nil {
				return nil, err
			}
		}

		if s != "" {
			tmp := stringToStringValue(s)
			val, err = addValues(val, tmp)
			if err != nil {
				return nil, err
			}
		}
	}

	return val, nil
}

func stringToListValue(s string) *bpparser.Value {
	list := strings.Fields(s)
	valList := make([]bpparser.Value, len(list))
	for i, l := range list {
		valList[i] = bpparser.Value{
			Type:        bpparser.String,
			StringValue: l,
		}
	}
	return &bpparser.Value{
		Type:      bpparser.List,
		ListValue: valList,
	}

}

func makeToListExpression(ms *mkparser.MakeString, scope mkparser.Scope) (*bpparser.Value, error) {
	fields := ms.Split(" \t")

	var listOfListValues []*bpparser.Value

	listValue := &bpparser.Value{
		Type: bpparser.List,
	}

	for _, f := range fields {
		if len(f.Variables) == 1 && f.Strings[0] == "" && f.Strings[1] == "" {
			if ret, ok := f.Variables[0].EvalFunction(scope); ok {
				listValue.ListValue = append(listValue.ListValue, bpparser.Value{
					Type:        bpparser.String,
					StringValue: ret,
				})
			} else {
				// Variable by itself, variable is probably a list
				if !f.Variables[0].Name.Const() {
					return nil, fmt.Errorf("unsupported non-const variable name")
				}
				if len(listValue.ListValue) > 0 {
					listOfListValues = append(listOfListValues, listValue)
				}
				listOfListValues = append(listOfListValues, &bpparser.Value{
					Type:     bpparser.List,
					Variable: f.Variables[0].Name.Value(nil),
				})
				listValue = &bpparser.Value{
					Type: bpparser.List,
				}
			}
		} else {
			s, err := makeToStringExpression(f, scope)
			if err != nil {
				return nil, err
			}
			if s == nil {
				continue
			}

			listValue.ListValue = append(listValue.ListValue, *s)
		}
	}

	if len(listValue.ListValue) > 0 {
		listOfListValues = append(listOfListValues, listValue)
	}

	if len(listOfListValues) == 0 {
		return listValue, nil
	}

	val := listOfListValues[0]
	for _, tmp := range listOfListValues[1:] {
		var err error
		val, err = addValues(val, tmp)
		if err != nil {
			return nil, err
		}
	}

	return val, nil
}

func stringToBoolValue(s string) (*bpparser.Value, error) {
	var b bool
	s = strings.TrimSpace(s)
	switch s {
	case "true":
		b = true
	case "false", "":
		b = false
	case "-frtti": // HACK for LOCAL_RTTI_VALUE
		b = true
	default:
		return nil, fmt.Errorf("unexpected bool value %s", s)
	}
	return &bpparser.Value{
		Type:      bpparser.Bool,
		BoolValue: b,
	}, nil
}

func makeToBoolExpression(ms *mkparser.MakeString) (*bpparser.Value, error) {
	if !ms.Const() {
		if len(ms.Variables) == 1 && ms.Strings[0] == "" && ms.Strings[1] == "" {
			name := ms.Variables[0].Name
			if !name.Const() {
				return nil, fmt.Errorf("unsupported non-const variable name")
			}
			return &bpparser.Value{
				Type:     bpparser.Bool,
				Variable: name.Value(nil),
			}, nil
		} else {
			return nil, fmt.Errorf("non-const bool expression %s", ms.Dump())
		}
	}

	return stringToBoolValue(ms.Value(nil))
}
