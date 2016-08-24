// Copyright 2015 Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package common

import (
	"github.com/google/blueprint"
	"github.com/google/blueprint/proptools"
)

type defaultsProperties struct {
	Defaults []string
}

type DefaultableModule struct {
	defaultsProperties    defaultsProperties
	defaultableProperties []interface{}
}

func (d *DefaultableModule) defaults() *defaultsProperties {
	return &d.defaultsProperties
}

func (d *DefaultableModule) setProperties(props []interface{}) {
	d.defaultableProperties = props
}

type Defaultable interface {
	defaults() *defaultsProperties
	setProperties([]interface{})
	applyDefaults(AndroidTopDownMutatorContext, Defaults)
}

var _ Defaultable = (*DefaultableModule)(nil)

func InitDefaultableModule(module AndroidModule, d Defaultable,
	props ...interface{}) (blueprint.Module, []interface{}) {

	d.setProperties(props)

	props = append(props, d.defaults())

	return module, props
}

type DefaultsModule struct {
	defaultProperties []interface{}
}

type Defaults interface {
	isDefaults() bool
	setProperties([]interface{})
	properties() []interface{}
}

func (d *DefaultsModule) isDefaults() bool {
	return true
}

func (d *DefaultsModule) properties() []interface{} {
	return d.defaultProperties
}

func (d *DefaultsModule) setProperties(props []interface{}) {
	d.defaultProperties = props
}

func InitDefaultsModule(module AndroidModule, d Defaults, props ...interface{}) (blueprint.Module, []interface{}) {
	d.setProperties(props)

	return module, props
}

var _ Defaults = (*DefaultsModule)(nil)

func (defaultable *DefaultableModule) applyDefaults(ctx AndroidTopDownMutatorContext,
	defaults Defaults) {

	for _, prop := range defaultable.defaultableProperties {
		for _, def := range defaults.properties() {
			if proptools.TypeEqual(prop, def) {
				err := proptools.PrependProperties(prop, def, nil)
				if err != nil {
					if propertyErr, ok := err.(*proptools.ExtendPropertyError); ok {
						ctx.PropertyErrorf(propertyErr.Property, "%s", propertyErr.Err.Error())
					} else {
						panic(err)
					}
				}
			}
		}
	}
}

func defaultsDepsMutator(ctx AndroidBottomUpMutatorContext) {
	if defaultable, ok := ctx.Module().(Defaultable); ok {
		ctx.AddDependency(ctx.Module(), defaultable.defaults().Defaults...)
	}
}

func defaultsMutator(ctx AndroidTopDownMutatorContext) {
	if defaultable, ok := ctx.Module().(Defaultable); ok {
		for _, defaultsDep := range defaultable.defaults().Defaults {
			ctx.VisitDirectDeps(func(m blueprint.Module) {
				if ctx.OtherModuleName(m) == defaultsDep {
					if defaultsModule, ok := m.(Defaults); ok {
						defaultable.applyDefaults(ctx, defaultsModule)
					} else {
						ctx.PropertyErrorf("defaults", "module %s is not an defaults module",
							ctx.OtherModuleName(m))
					}
				}
			})
		}
	}
}
