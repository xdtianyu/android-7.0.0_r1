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
	"android/soong"

	"github.com/google/blueprint"
)

type AndroidTopDownMutator func(AndroidTopDownMutatorContext)

type AndroidTopDownMutatorContext interface {
	blueprint.TopDownMutatorContext
	androidBaseContext
}

type androidTopDownMutatorContext struct {
	blueprint.TopDownMutatorContext
	androidBaseContextImpl
}

type AndroidBottomUpMutator func(AndroidBottomUpMutatorContext)

type AndroidBottomUpMutatorContext interface {
	blueprint.BottomUpMutatorContext
	androidBaseContext
}

type androidBottomUpMutatorContext struct {
	blueprint.BottomUpMutatorContext
	androidBaseContextImpl
}

func RegisterBottomUpMutator(name string, mutator AndroidBottomUpMutator) {
	soong.RegisterBottomUpMutator(name, func(ctx blueprint.BottomUpMutatorContext) {
		if a, ok := ctx.Module().(AndroidModule); ok {
			actx := &androidBottomUpMutatorContext{
				BottomUpMutatorContext: ctx,
				androidBaseContextImpl: a.base().androidBaseContextFactory(ctx),
			}
			mutator(actx)
		}
	})
}

func RegisterTopDownMutator(name string, mutator AndroidTopDownMutator) {
	soong.RegisterTopDownMutator(name, func(ctx blueprint.TopDownMutatorContext) {
		if a, ok := ctx.Module().(AndroidModule); ok {
			actx := &androidTopDownMutatorContext{
				TopDownMutatorContext:  ctx,
				androidBaseContextImpl: a.base().androidBaseContextFactory(ctx),
			}
			mutator(actx)
		}
	})
}
