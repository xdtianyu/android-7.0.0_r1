/*
 * Copyright (C) 2015 The Android Open Source Project
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package android.databinding.tool.writer

import android.databinding.tool.BindingTarget
import android.databinding.tool.InverseBinding
import android.databinding.tool.LayoutBinder
import android.databinding.tool.expr.Expr
import android.databinding.tool.expr.ExprModel
import android.databinding.tool.expr.FieldAccessExpr
import android.databinding.tool.expr.IdentifierExpr
import android.databinding.tool.expr.ListenerExpr
import android.databinding.tool.expr.ResourceExpr
import android.databinding.tool.expr.TernaryExpr
import android.databinding.tool.ext.androidId
import android.databinding.tool.ext.br
import android.databinding.tool.ext.joinToCamelCaseAsVar
import android.databinding.tool.ext.lazyProp
import android.databinding.tool.ext.versionedLazy
import android.databinding.tool.processing.ErrorMessages
import android.databinding.tool.reflection.ModelAnalyzer
import android.databinding.tool.util.L
import java.util.ArrayList
import java.util.Arrays
import java.util.BitSet
import java.util.HashMap

fun String.stripNonJava() = this.split("[^a-zA-Z0-9]".toRegex()).map{ it.trim() }.joinToCamelCaseAsVar()

enum class Scope {
    FIELD,
    METHOD,
    FLAG,
    EXECUTE_PENDING_METHOD,
    CONSTRUCTOR_PARAM
}

class ExprModelExt {
    val usedFieldNames = hashMapOf<Scope, MutableSet<String>>();
    init {
        Scope.values().forEach { usedFieldNames[it] = hashSetOf<String>() }
    }
    val localizedFlags = arrayListOf<FlagSet>()

    fun localizeFlag(set : FlagSet, name:String) : FlagSet {
        localizedFlags.add(set)
        val result = getUniqueName(name, Scope.FLAG, false)
        set.localName = result
        return set
    }

    fun getUniqueName(base : String, scope : Scope, isPublic : kotlin.Boolean) : String {
        var candidateBase = base
        if (!isPublic && candidateBase.length > 20) {
            candidateBase = candidateBase.substring(0, 20);
        }
        var candidate = candidateBase
        var i = 0
        while (usedFieldNames[scope]!!.contains(candidate)) {
            i ++
            candidate = candidateBase + i
        }
        usedFieldNames[scope]!!.add(candidate)
        return candidate
    }
}

val ExprModel.ext by lazyProp { target : ExprModel ->
    ExprModelExt()
}

fun ExprModel.getUniqueFieldName(base : String, isPublic : kotlin.Boolean) : String = ext.getUniqueName(base, Scope.FIELD, isPublic)
fun ExprModel.getUniqueMethodName(base : String, isPublic : kotlin.Boolean) : String = ext.getUniqueName(base, Scope.METHOD, isPublic)
fun ExprModel.getConstructorParamName(base : String) : String = ext.getUniqueName(base, Scope.CONSTRUCTOR_PARAM, false)

fun ExprModel.localizeFlag(set : FlagSet, base : String) : FlagSet = ext.localizeFlag(set, base)

val Expr.needsLocalField by lazyProp { expr : Expr ->
    expr.canBeEvaluatedToAVariable() && !(expr.isVariable() && !expr.isUsed) && (expr.isDynamic || expr is ResourceExpr)
}


// not necessarily unique. Uniqueness is solved per scope
val BindingTarget.readableName by lazyProp { target: BindingTarget ->
    if (target.id == null) {
        "boundView" + indexFromTag(target.tag)
    } else {
        target.id.androidId().stripNonJava()
    }
}

fun BindingTarget.superConversion(variable : String) : String {
    if (resolvedType != null && resolvedType.extendsViewStub()) {
        return "new android.databinding.ViewStubProxy((android.view.ViewStub) $variable)"
    } else {
        return "($interfaceClass) $variable"
    }
}

val BindingTarget.fieldName : String by lazyProp { target : BindingTarget ->
    val name : String
    val isPublic : kotlin.Boolean
    if (target.id == null) {
        name = "m${target.readableName}"
        isPublic = false
    } else {
        name = target.readableName
        isPublic = true
    }
    target.model.getUniqueFieldName(name, isPublic)
}

val BindingTarget.androidId by lazyProp { target : BindingTarget ->
    if (target.id.startsWith("@android:id/")) {
        "android.R.id.${target.id.androidId()}"
    } else {
        "R.id.${target.id.androidId()}"
    }
}

val BindingTarget.interfaceClass by lazyProp { target : BindingTarget ->
    if (target.resolvedType != null && target.resolvedType.extendsViewStub()) {
        "android.databinding.ViewStubProxy"
    } else {
        target.interfaceType
    }
}

val BindingTarget.constructorParamName by lazyProp { target : BindingTarget ->
    target.model.getConstructorParamName(target.readableName)
}

// not necessarily unique. Uniqueness is decided per scope
val Expr.readableName by lazyProp { expr : Expr ->
    val stripped = "${expr.uniqueKey.stripNonJava()}"
    L.d("readableUniqueName for [%s] %s is %s", System.identityHashCode(expr), expr.uniqueKey, stripped)
    stripped
}

val Expr.fieldName by lazyProp { expr : Expr ->
    expr.model.getUniqueFieldName("m${expr.readableName.capitalize()}", false)
}

val InverseBinding.fieldName by lazyProp { inverseBinding : InverseBinding ->
    val targetName = inverseBinding.target.fieldName;
    val eventName = inverseBinding.eventAttribute.stripNonJava()
    inverseBinding.model.getUniqueFieldName("$targetName$eventName", false)
}

val Expr.listenerClassName by lazyProp { expr : Expr ->
    expr.model.getUniqueFieldName("${expr.resolvedType.simpleName}Impl", false)
}

val Expr.oldValueName by lazyProp { expr : Expr ->
    expr.model.getUniqueFieldName("mOld${expr.readableName.capitalize()}", false)
}

val Expr.executePendingLocalName by lazyProp { expr : Expr ->
    if(expr.needsLocalField) "${expr.model.ext.getUniqueName(expr.readableName, Scope.EXECUTE_PENDING_METHOD, false)}"
    else expr.toCode().generate()
}

val Expr.setterName by lazyProp { expr : Expr ->
    expr.model.getUniqueMethodName("set${expr.readableName.capitalize()}", true)
}

val Expr.onChangeName by lazyProp { expr : Expr ->
    expr.model.getUniqueMethodName("onChange${expr.readableName.capitalize()}", false)
}

val Expr.getterName by lazyProp { expr : Expr ->
    expr.model.getUniqueMethodName("get${expr.readableName.capitalize()}", true)
}

fun Expr.isVariable() = this is IdentifierExpr && this.isDynamic

val Expr.dirtyFlagSet by lazyProp { expr : Expr ->
    FlagSet(expr.invalidFlags, expr.model.flagBucketCount)
}

val Expr.invalidateFlagSet by lazyProp { expr : Expr ->
    FlagSet(expr.id)
}

val Expr.shouldReadFlagSet by versionedLazy { expr : Expr ->
    FlagSet(expr.shouldReadFlags, expr.model.flagBucketCount)
}

val Expr.shouldReadWithConditionalsFlagSet by versionedLazy { expr : Expr ->
    FlagSet(expr.shouldReadFlagsWithConditionals, expr.model.flagBucketCount)
}

val Expr.conditionalFlags by lazyProp { expr : Expr ->
    arrayListOf(FlagSet(expr.getRequirementFlagIndex(false)),
            FlagSet(expr.getRequirementFlagIndex(true)))
}

val LayoutBinder.requiredComponent by lazyProp { layoutBinder: LayoutBinder ->
    val requiredFromBindings = layoutBinder.
            bindingTargets.
            flatMap { it.bindings }.
            firstOrNull { it.bindingAdapterInstanceClass != null }?.bindingAdapterInstanceClass
    val requiredFromInverse = layoutBinder.
            bindingTargets.
            flatMap { it.inverseBindings }.
            firstOrNull { it.bindingAdapterInstanceClass != null }?.bindingAdapterInstanceClass
    requiredFromBindings ?: requiredFromInverse
}

fun Expr.getRequirementFlagSet(expected : Boolean) : FlagSet = conditionalFlags[if(expected) 1 else 0]

fun FlagSet.notEmpty(cb : (suffix : String, value : Long) -> Unit) {
    buckets.withIndex().forEach {
        if (it.value != 0L) {
            cb(getWordSuffix(it.index), buckets[it.index])
        }
    }
}

fun getWordSuffix(wordIndex : Int) : String {
    return if(wordIndex == 0) "" else "_$wordIndex"
}

fun FlagSet.localValue(bucketIndex : Int) =
        if (localName == null) binaryCode(bucketIndex)
        else "$localName${getWordSuffix(bucketIndex)}"

fun FlagSet.binaryCode(bucketIndex : Int) = longToBinary(buckets[bucketIndex])


fun longToBinary(l : Long) = "0x${java.lang.Long.toHexString(l)}L"

fun <T> FlagSet.mapOr(other : FlagSet, cb : (suffix : String, index : Int) -> T) : List<T> {
    val min = Math.min(buckets.size, other.buckets.size)
    val result = arrayListOf<T>()
    for (i in 0..(min - 1)) {
        // if these two can match by any chance, call the callback
        if (intersect(other, i)) {
            result.add(cb(getWordSuffix(i), i))
        }
    }
    return result
}

fun indexFromTag(tag : String) : kotlin.Int {
    val startIndex : kotlin.Int
    if (tag.startsWith("binding_")) {
        startIndex = "binding_".length;
    } else {
        startIndex = tag.lastIndexOf('_') + 1
    }
    return Integer.parseInt(tag.substring(startIndex))
}

class LayoutBinderWriter(val layoutBinder : LayoutBinder) {
    val model = layoutBinder.model
    val indices = HashMap<BindingTarget, kotlin.Int>()
    val mDirtyFlags by lazy {
        val fs = FlagSet(BitSet(), model.flagBucketCount);
        Arrays.fill(fs.buckets, -1)
        fs.isDynamic = true
        model.localizeFlag(fs, "mDirtyFlags")
        fs
    }

    val className = layoutBinder.implementationName

    val baseClassName = "${layoutBinder.className}"

    val includedBinders by lazy {
        layoutBinder.bindingTargets.filter { it.isBinder }
    }

    val variables by lazy {
        model.exprMap.values.filterIsInstance(IdentifierExpr::class.java).filter { it.isVariable() }
    }

    val usedVariables by lazy {
        variables.filter {it.isUsed }
    }

    public fun write(minSdk : kotlin.Int) : String  {
        layoutBinder.resolveWhichExpressionsAreUsed()
        calculateIndices();
        return kcode("package ${layoutBinder.`package`};") {
            nl("import ${layoutBinder.modulePackage}.R;")
            nl("import ${layoutBinder.modulePackage}.BR;")
            nl("import android.view.View;")
            val classDeclaration : String
            if (layoutBinder.hasVariations()) {
                classDeclaration = "$className extends $baseClassName"
            } else {
                classDeclaration = "$className extends android.databinding.ViewDataBinding"
            }
            nl("public class $classDeclaration {") {
                tab(declareIncludeViews())
                tab(declareViews())
                tab(declareVariables())
                tab(declareBoundValues())
                tab(declareListeners())
                tab(declareInverseBindingImpls());
                tab(declareConstructor(minSdk))
                tab(declareInvalidateAll())
                tab(declareHasPendingBindings())
                tab(declareSetVariable())
                tab(variableSettersAndGetters())
                tab(onFieldChange())

                tab(executePendingBindings())

                tab(declareListenerImpls())
                tab(declareDirtyFlags())
                if (!layoutBinder.hasVariations()) {
                    tab(declareFactories())
                }
            }
            nl("}")
            tab(flagMapping())
            tab("//end")
        }.generate()
    }
    fun calculateIndices() : Unit {
        val taggedViews = layoutBinder.bindingTargets.filter{
            it.isUsed && it.tag != null && !it.isBinder
        }
        taggedViews.forEach {
            indices.put(it, indexFromTag(it.tag))
        }
        val indexStart = maxIndex() + 1
        layoutBinder.bindingTargets.filter{
            it.isUsed && !taggedViews.contains(it)
        }.withIndex().forEach {
            indices.put(it.value, it.index + indexStart)
        }
    }
    fun declareIncludeViews() = kcode("") {
        nl("private static final android.databinding.ViewDataBinding.IncludedLayouts sIncludes;")
        nl("private static final android.util.SparseIntArray sViewsWithIds;")
        nl("static {") {
            val hasBinders = layoutBinder.bindingTargets.firstOrNull{ it.isUsed && it.isBinder } != null
            if (!hasBinders) {
                tab("sIncludes = null;")
            } else {
                val numBindings = layoutBinder.bindingTargets.filter{ it.isUsed }.count()
                tab("sIncludes = new android.databinding.ViewDataBinding.IncludedLayouts($numBindings);")
                val includeMap = HashMap<BindingTarget, ArrayList<BindingTarget>>()
                layoutBinder.bindingTargets.filter{ it.isUsed && it.isBinder }.forEach {
                    val includeTag = it.tag;
                    val parent = layoutBinder.bindingTargets.firstOrNull {
                        it.isUsed && !it.isBinder && includeTag.equals(it.tag)
                    } ?: throw IllegalStateException("Could not find parent of include file")
                    var list = includeMap[parent]
                    if (list == null) {
                        list = ArrayList<BindingTarget>()
                        includeMap.put(parent, list)
                    }
                    list.add(it)
                }

                includeMap.keys.forEach {
                    val index = indices[it]
                    tab("sIncludes.setIncludes($index, ") {
                        tab ("new String[] {${
                        includeMap[it]!!.map {
                            "\"${it.includedLayout}\""
                        }.joinToString(", ")
                        }},")
                        tab("new int[] {${
                        includeMap[it]!!.map {
                            "${indices[it]}"
                        }.joinToString(", ")
                        }},")
                        tab("new int[] {${
                        includeMap[it]!!.map {
                            "R.layout.${it.includedLayout}"
                        }.joinToString(", ")
                        }});")
                    }
                }
            }
            val viewsWithIds = layoutBinder.bindingTargets.filter {
                it.isUsed && !it.isBinder && (!it.supportsTag() || (it.id != null && it.tag == null))
            }
            if (viewsWithIds.isEmpty()) {
                tab("sViewsWithIds = null;")
            } else {
                tab("sViewsWithIds = new android.util.SparseIntArray();")
                viewsWithIds.forEach {
                    tab("sViewsWithIds.put(${it.androidId}, ${indices[it]});")
                }
            }
        }
        nl("}")
    }

    fun maxIndex() : kotlin.Int {
        val maxIndex = indices.values.max()
        if (maxIndex == null) {
            return -1
        } else {
            return maxIndex
        }
    }

    fun declareConstructor(minSdk : kotlin.Int) = kcode("") {
        val bindingCount = maxIndex() + 1
        val parameterType : String
        val superParam : String
        if (layoutBinder.isMerge) {
            parameterType = "View[]"
            superParam = "root[0]"
        } else {
            parameterType = "View"
            superParam = "root"
        }
        val rootTagsSupported = minSdk >= 14
        if (layoutBinder.hasVariations()) {
            nl("")
            nl("public $className(android.databinding.DataBindingComponent bindingComponent, $parameterType root) {") {
                tab("this(bindingComponent, $superParam, mapBindings(bindingComponent, root, $bindingCount, sIncludes, sViewsWithIds));")
            }
            nl("}")
            nl("private $className(android.databinding.DataBindingComponent bindingComponent, $parameterType root, Object[] bindings) {") {
                tab("super(bindingComponent, $superParam, ${model.observables.size}") {
                    layoutBinder.sortedTargets.filter { it.id != null }.forEach {
                        tab(", ${fieldConversion(it)}")
                    }
                    tab(");")
                }
            }
        } else {
            nl("public $baseClassName(android.databinding.DataBindingComponent bindingComponent, $parameterType root) {") {
                tab("super(bindingComponent, $superParam, ${model.observables.size});")
                tab("final Object[] bindings = mapBindings(bindingComponent, root, $bindingCount, sIncludes, sViewsWithIds);")
            }
        }
        if (layoutBinder.requiredComponent != null) {
            tab("ensureBindingComponentIsNotNull(${layoutBinder.requiredComponent}.class);")
        }
        val taggedViews = layoutBinder.sortedTargets.filter{it.isUsed }
        taggedViews.forEach {
            if (!layoutBinder.hasVariations() || it.id == null) {
                tab("this.${it.fieldName} = ${fieldConversion(it)};")
            }
            if (!it.isBinder) {
                if (it.resolvedType != null && it.resolvedType.extendsViewStub()) {
                    tab("this.${it.fieldName}.setContainingBinding(this);")
                }
                if (it.supportsTag() && it.tag != null &&
                        (rootTagsSupported || it.tag.startsWith("binding_"))) {
                    val originalTag = it.originalTag;
                    var tagValue = "null"
                    if (originalTag != null && !originalTag.startsWith("@{")) {
                        tagValue = "\"$originalTag\""
                        if (originalTag.startsWith("@")) {
                            var packageName = layoutBinder.modulePackage
                            if (originalTag.startsWith("@android:")) {
                                packageName = "android"
                            }
                            val slashIndex = originalTag.indexOf('/')
                            val resourceId = originalTag.substring(slashIndex + 1)
                            tagValue = "root.getResources().getString($packageName.R.string.$resourceId)"
                        }
                    }
                    tab("this.${it.fieldName}.setTag($tagValue);")
                } else if (it.tag != null && !it.tag.startsWith("binding_") &&
                    it.originalTag != null) {
                    L.e(ErrorMessages.ROOT_TAG_NOT_SUPPORTED, it.originalTag)
                }
            }
        }
        tab("setRootTag(root);")
        tab("invalidateAll();");
        nl("}")
    }

    fun fieldConversion(target : BindingTarget) : String {
        if (!target.isUsed) {
            return "null"
        } else {
            val index = indices[target] ?: throw IllegalStateException("Unknown binding target")
            val variableName = "bindings[$index]"
            return target.superConversion(variableName)
        }
    }

    fun declareInvalidateAll() = kcode("") {
        nl("@Override")
        nl("public void invalidateAll() {") {
            val fs = FlagSet(layoutBinder.model.invalidateAnyBitSet,
                    layoutBinder.model.flagBucketCount);
            tab("synchronized(this) {") {
                for (i in (0..(mDirtyFlags.buckets.size - 1))) {
                    tab("${mDirtyFlags.localValue(i)} = ${fs.localValue(i)};")
                }
            } tab("}")
            includedBinders.filter{it.isUsed }.forEach { binder ->
                tab("${binder.fieldName}.invalidateAll();")
            }
            tab("requestRebind();");
        }
        nl("}")
    }

    fun declareHasPendingBindings()  = kcode("") {
        nl("@Override")
        nl("public boolean hasPendingBindings() {") {
            if (mDirtyFlags.buckets.size > 0) {
                tab("synchronized(this) {") {
                    val flagCheck = 0.rangeTo(mDirtyFlags.buckets.size - 1).map {
                            "${mDirtyFlags.localValue(it)} != 0"
                    }.joinToString(" || ")
                    tab("if ($flagCheck) {") {
                        tab("return true;")
                    }
                    tab("}")
                }
                tab("}")
            }
            includedBinders.filter{it.isUsed }.forEach { binder ->
                tab("if (${binder.fieldName}.hasPendingBindings()) {") {
                    tab("return true;")
                }
                tab("}")
            }
            tab("return false;")
        }
        nl("}")
    }

    fun declareSetVariable() = kcode("") {
        nl("public boolean setVariable(int variableId, Object variable) {") {
            tab("switch(variableId) {") {
                usedVariables.forEach {
                    tab ("case ${it.name.br()} :") {
                        tab("${it.setterName}((${it.resolvedType.toJavaCode()}) variable);")
                        tab("return true;")
                    }
                }
                val declaredOnly = variables.filter { !it.isUsed && it.isDeclared };
                declaredOnly.forEachIndexed { i, identifierExpr ->
                    tab ("case ${identifierExpr.name.br()} :") {
                        if (i == declaredOnly.size - 1) {
                            tab("return true;")
                        }
                    }
                }
            }
            tab("}")
            tab("return false;")
        }
        nl("}")
    }

    fun variableSettersAndGetters() = kcode("") {
        variables.filterNot{it.isUsed }.forEach {
            nl("public void ${it.setterName}(${it.resolvedType.toJavaCode()} ${it.readableName}) {") {
                tab("// not used, ignore")
            }
            nl("}")
            nl("")
            nl("public ${it.resolvedType.toJavaCode()} ${it.getterName}() {") {
                tab("return ${it.defaultValue};")
            }
            nl("}")
        }
        usedVariables.forEach {
            if (it.userDefinedType != null) {
                nl("public void ${it.setterName}(${it.resolvedType.toJavaCode()} ${it.readableName}) {") {
                    if (it.isObservable) {
                        tab("updateRegistration(${it.id}, ${it.readableName});");
                    }
                    tab("this.${it.fieldName} = ${it.readableName};")
                    // set dirty flags!
                    val flagSet = it.invalidateFlagSet
                    tab("synchronized(this) {") {
                        mDirtyFlags.mapOr(flagSet) { suffix, index ->
                            tab("${mDirtyFlags.localName}$suffix |= ${flagSet.localValue(index)};")
                        }
                    } tab ("}")
                    // TODO: Remove this condition after releasing version 1.1 of SDK
                    if (ModelAnalyzer.getInstance().findClass("android.databinding.ViewDataBinding", null).isObservable) {
                        tab("notifyPropertyChanged(${it.name.br()});")
                    }
                    tab("super.requestRebind();")
                }
                nl("}")
                nl("")
                nl("public ${it.resolvedType.toJavaCode()} ${it.getterName}() {") {
                    tab("return ${it.fieldName};")
                }
                nl("}")
            }
        }
    }

    fun onFieldChange() = kcode("") {
        nl("@Override")
        nl("protected boolean onFieldChange(int localFieldId, Object object, int fieldId) {") {
            tab("switch (localFieldId) {") {
                model.observables.forEach {
                    tab("case ${it.id} :") {
                        tab("return ${it.onChangeName}((${it.resolvedType.toJavaCode()}) object, fieldId);")
                    }
                }
            }
            tab("}")
            tab("return false;")
        }
        nl("}")
        nl("")

        model.observables.forEach {
            nl("private boolean ${it.onChangeName}(${it.resolvedType.toJavaCode()} ${it.readableName}, int fieldId) {") {
                tab("switch (fieldId) {", {
                    val accessedFields: List<FieldAccessExpr> = it.parents.filterIsInstance(FieldAccessExpr::class.java)
                    accessedFields.filter { it.hasBindableAnnotations() }
                            .groupBy { it.brName }
                            .forEach {
                                // If two expressions look different but resolve to the same method,
                                // we are not yet able to merge them. This is why we merge their
                                // flags below.
                                tab("case ${it.key}:") {
                                    tab("synchronized(this) {") {
                                        val flagSet = it.value.foldRight(FlagSet()) { l, r -> l.invalidateFlagSet.or(r) }

                                        mDirtyFlags.mapOr(flagSet) { suffix, index ->
                                            tab("${mDirtyFlags.localValue(index)} |= ${flagSet.localValue(index)};")
                                        }
                                    } tab("}")
                                    tab("return true;")
                                }

                            }
                    tab("case ${"".br()}:") {
                        val flagSet = it.invalidateFlagSet
                        tab("synchronized(this) {") {
                            mDirtyFlags.mapOr(flagSet) { suffix, index ->
                                tab("${mDirtyFlags.localName}$suffix |= ${flagSet.localValue(index)};")
                            }
                        } tab("}")
                        tab("return true;")
                    }

                })
                tab("}")
                tab("return false;")
            }
            nl("}")
            nl("")
        }
    }

    fun declareViews() = kcode("// views") {
        val oneLayout = !layoutBinder.hasVariations();
        layoutBinder.sortedTargets.filter {it.isUsed && (oneLayout || it.id == null)}.forEach {
            val access : String
            if (oneLayout && it.id != null) {
                access = "public"
            } else {
                access = "private"
            }
            nl("$access final ${it.interfaceClass} ${it.fieldName};")
        }
    }

    fun declareVariables() = kcode("// variables") {
        usedVariables.forEach {
            nl("private ${it.resolvedType.toJavaCode()} ${it.fieldName};")
        }
    }

    fun declareBoundValues() = kcode("// values") {
        layoutBinder.sortedTargets.filter { it.isUsed }
                .flatMap { it.bindings }
                .filter { it.requiresOldValue() }
                .flatMap{ it.componentExpressions.toArrayList() }
                .groupBy { it }
                .forEach {
                    val expr = it.key
                    nl("private ${expr.resolvedType.toJavaCode()} ${expr.oldValueName};")
                }
    }

    fun declareListeners() = kcode("// listeners") {
        model.exprMap.values.filter {
            it is ListenerExpr
        }.groupBy { it }.forEach {
            val expr = it.key as ListenerExpr
            nl("private ${expr.listenerClassName} ${expr.fieldName};")
        }
    }

    fun declareInverseBindingImpls() = kcode("// Inverse Binding Event Handlers") {
        layoutBinder.sortedTargets.filter { it.isUsed }.forEach { target ->
            target.inverseBindings.forEach { inverseBinding ->
                val className : String
                val param : String
                if (inverseBinding.isOnBinder) {
                    className = "android.databinding.ViewDataBinding.PropertyChangedInverseListener"
                    param = "BR.${inverseBinding.eventAttribute}"
                } else {
                    className = "android.databinding.InverseBindingListener"
                    param = ""
                }
                nl("private $className ${inverseBinding.fieldName} = new $className($param) {") {
                    tab("@Override")
                    tab("public void onChange() {") {
                        tab(inverseBinding.toJavaCode("mBindingComponent", mDirtyFlags)).app(";");
                    }
                    tab("}")
                }
                nl("};")
            }
        }
    }
    fun declareDirtyFlags() = kcode("// dirty flag") {
        model.ext.localizedFlags.forEach { flag ->
            flag.notEmpty { suffix, value ->
                nl("private")
                app(" ", if(flag.isDynamic) null else "static final");
                app(" ", " ${flag.type} ${flag.localName}$suffix = ${longToBinary(value)};")
            }
        }
    }

    fun flagMapping() = kcode("/* flag mapping") {
        if (model.flagMapping != null) {
            val mapping = model.flagMapping
            for (i in mapping.indices) {
                tab("flag $i: ${mapping[i]}")
            }
        }
        nl("flag mapping end*/")
    }

    fun executePendingBindings() = kcode("") {
        nl("@Override")
        nl("protected void executeBindings() {") {
            val tmpDirtyFlags = FlagSet(mDirtyFlags.buckets)
            tmpDirtyFlags.localName = "dirtyFlags";
            for (i in (0..mDirtyFlags.buckets.size - 1)) {
                tab("${tmpDirtyFlags.type} ${tmpDirtyFlags.localValue(i)} = 0;")
            }
            tab("synchronized(this) {") {
                for (i in (0..mDirtyFlags.buckets.size - 1)) {
                    tab("${tmpDirtyFlags.localValue(i)} = ${mDirtyFlags.localValue(i)};")
                    tab("${mDirtyFlags.localValue(i)} = 0;")
                }
            } tab("}")
            model.pendingExpressions.filter { it.needsLocalField }.forEach {
                tab("${it.resolvedType.toJavaCode()} ${it.executePendingLocalName} = ${if (it.isVariable()) it.fieldName else it.defaultValue};")
            }
            L.d("writing executePendingBindings for %s", className)
            do {
                val batch = ExprModel.filterShouldRead(model.pendingExpressions).toArrayList()
                val justRead = arrayListOf<Expr>()
                L.d("batch: %s", batch)
                while (!batch.none()) {
                    val readNow = batch.filter { it.shouldReadNow(justRead) }
                    if (readNow.isEmpty()) {
                        throw IllegalStateException("do not know what I can read. bailing out ${batch.joinToString("\n")}")
                    }
                    L.d("new read now. batch size: %d, readNow size: %d", batch.size, readNow.size)
                    nl(readWithDependants(readNow, justRead, batch, tmpDirtyFlags))
                    batch.removeAll(justRead)
                }
                tab("// batch finished")
            } while (model.markBitsRead())
            // verify everything is read.
            val batch = ExprModel.filterShouldRead(model.pendingExpressions).toArrayList()
            if (batch.isNotEmpty()) {
                L.e("could not generate code for %s. This might be caused by circular dependencies."
                        + "Please report on b.android.com. %d %s %s", layoutBinder.layoutname,
                        batch.size, batch[0], batch[0].toCode().generate())
            }
            //
            layoutBinder.sortedTargets.filter { it.isUsed }
                    .flatMap { it.bindings }
                    .groupBy {
                        "${tmpDirtyFlags.mapOr(it.expr.dirtyFlagSet) { suffix, index ->
                            "(${tmpDirtyFlags.localValue(index)} & ${it.expr.dirtyFlagSet.localValue(index)}) != 0"
                        }.joinToString(" || ") }"
                    }.forEach {
                tab("if (${it.key}) {") {
                    it.value.groupBy { Math.max(1, it.minApi) }.forEach {
                        val setterValues = kcode("") {
                            it.value.forEach { binding ->
                                val fieldName: String
                                if (binding.target.viewClass.
                                        equals(binding.target.interfaceType)) {
                                    fieldName = "this.${binding.target.fieldName}"
                                } else {
                                    fieldName = "((${binding.target.viewClass}) this.${binding.target.fieldName})"
                                }
                                tab(binding.toJavaCode(fieldName, "this.mBindingComponent")).app(";")
                            }
                        }
                        tab("// api target ${it.key}")
                        if (it.key > 1) {
                            tab("if(getBuildSdkInt() >= ${it.key}) {") {
                                app("", setterValues)
                            }
                            tab("}")
                        } else {
                            app("", setterValues)
                        }
                    }
                }
                tab("}")
            }


            layoutBinder.sortedTargets.filter { it.isUsed }
                    .flatMap { it.bindings }
                    .filter { it.requiresOldValue() }
                    .groupBy {"${tmpDirtyFlags.mapOr(it.expr.dirtyFlagSet) { suffix, index ->
                        "(${tmpDirtyFlags.localValue(index)} & ${it.expr.dirtyFlagSet.localValue(index)}) != 0"
                    }.joinToString(" || ")
                    }"}.forEach {
                tab("if (${it.key}) {") {
                    it.value.groupBy { it.expr }.map { it.value.first() }.forEach {
                        it.componentExpressions.forEach { expr ->
                            tab("this.${expr.oldValueName} = ${expr.toCode().generate()};")
                        }
                    }
                }
                tab("}")
            }
            includedBinders.filter{it.isUsed }.forEach { binder ->
                tab("${binder.fieldName}.executePendingBindings();")
            }
            layoutBinder.sortedTargets.filter{
                it.isUsed && it.resolvedType != null && it.resolvedType.extendsViewStub()
            }.forEach {
                tab("if (${it.fieldName}.getBinding() != null) {") {
                    tab("${it.fieldName}.getBinding().executePendingBindings();")
                }
                tab("}")
            }
        }
        nl("}")
    }

    fun readWithDependants(expressionList: List<Expr>, justRead: MutableList<Expr>,
            batch: MutableList<Expr>, tmpDirtyFlags: FlagSet,
            inheritedFlags: FlagSet? = null) : KCode = kcode("") {
        expressionList.groupBy { it.shouldReadFlagSet }.forEach {
            val flagSet = it.key
            val needsIfWrapper = inheritedFlags == null || !flagSet.bitsEqual(inheritedFlags)
            val expressions = it.value
            val ifClause = "if (${tmpDirtyFlags.mapOr(flagSet){ suffix, index ->
                "(${tmpDirtyFlags.localValue(index)} & ${flagSet.localValue(index)}) != 0"
            }.joinToString(" || ")
            })"
            val readCode = kcode("") {
                val dependants = ArrayList<Expr>()
                expressions.groupBy { condition(it) }.forEach {
                    val condition = it.key
                    val assignedValues = it.value.filter { it.needsLocalField && !it.isVariable() }
                    if (!assignedValues.isEmpty()) {
                        val assignment = kcode("") {
                            assignedValues.forEach { expr: Expr ->
                                tab("// read ${expr.uniqueKey}")
                                tab("${expr.executePendingLocalName}").app(" = ", expr.toFullCode()).app(";")
                            }
                        }
                        if (condition != null) {
                            tab("if ($condition) {") {
                                app("", assignment)
                            }
                            tab ("}")
                        } else {
                            app("", assignment)
                        }
                        it.value.filter { it.isObservable }.forEach { expr: Expr ->
                            tab("updateRegistration(${expr.id}, ${expr.executePendingLocalName});")
                        }
                    }

                    it.value.forEach { expr: Expr ->
                        justRead.add(expr)
                        L.d("%s / readWithDependants %s", className, expr.uniqueKey);
                        L.d("flag set:%s . inherited flags: %s. need another if: %s", flagSet, inheritedFlags, needsIfWrapper);

                        // if I am the condition for an expression, set its flag
                        expr.dependants.filter {
                            !it.isConditional && it.dependant is TernaryExpr &&
                                    (it.dependant as TernaryExpr).pred == expr
                        }.map { it.dependant }.groupBy {
                            // group by when those ternaries will be evaluated (e.g. don't set conditional flags for no reason)
                            val ternaryBitSet = it.shouldReadFlagsWithConditionals
                            val isBehindTernary = ternaryBitSet.nextSetBit(model.invalidateAnyFlagIndex) == -1
                            if (!isBehindTernary) {
                                val ternaryFlags = it.shouldReadWithConditionalsFlagSet
                                "if(${tmpDirtyFlags.mapOr(ternaryFlags){ suffix, index ->
                                    "(${tmpDirtyFlags.localValue(index)} & ${ternaryFlags.localValue(index)}) != 0"
                                }.joinToString(" || ")}) {"
                            } else {
                                // TODO if it is behind a ternary, we should set it when its predicate is elevated
                                // Normally, this would mean that there is another code path to re-read our current expression.
                                // Unfortunately, this may not be true due to the coverage detection in `expr#markAsReadIfDone`, this may never happen.
                                // for v1.0, we'll go with always setting it and suffering an unnecessary calculation for this edge case.
                                // we can solve this by listening to elevation events from the model.
                                ""
                            }
                        }.forEach {
                            val hasAnotherIf = it.key != ""
                            if (hasAnotherIf) {
                                tab(it.key) {
                                    tab("if (${expr.executePendingLocalName}) {") {
                                        it.value.forEach {
                                            val set = it.getRequirementFlagSet(true)
                                            mDirtyFlags.mapOr(set) { suffix, index ->
                                                tab("${tmpDirtyFlags.localValue(index)} |= ${set.localValue(index)};")
                                            }
                                        }
                                    }
                                    tab("} else {") {
                                        it.value.forEach {
                                            val set = it.getRequirementFlagSet(false)
                                            mDirtyFlags.mapOr(set) { suffix, index ->
                                                tab("${tmpDirtyFlags.localValue(index)} |= ${set.localValue(index)};")
                                            }
                                        }
                                    }.tab("}")
                                }.app("}")
                            } else {
                                tab("if (${expr.executePendingLocalName}) {") {
                                    it.value.forEach {
                                        val set = it.getRequirementFlagSet(true)
                                        mDirtyFlags.mapOr(set) { suffix, index ->
                                            tab("${tmpDirtyFlags.localValue(index)} |= ${set.localValue(index)};")
                                        }
                                    }
                                }
                                tab("} else {") {
                                    it.value.forEach {
                                        val set = it.getRequirementFlagSet(false)
                                        mDirtyFlags.mapOr(set) { suffix, index ->
                                            tab("${tmpDirtyFlags.localValue(index)} |= ${set.localValue(index)};")
                                        }
                                    }
                                } app("}")
                            }
                        }
                        val chosen = expr.dependants.filter {
                            val dependant = it.dependant
                            batch.contains(dependant) &&
                                    dependant.shouldReadFlagSet.andNot(flagSet).isEmpty &&
                                    dependant.shouldReadNow(justRead)
                        }
                        if (chosen.isNotEmpty()) {
                            dependants.addAll(chosen.map { it.dependant })
                        }
                    }
                }
                if (dependants.isNotEmpty()) {
                    val nextInheritedFlags = if (needsIfWrapper) flagSet else inheritedFlags
                    nl(readWithDependants(dependants, justRead, batch, tmpDirtyFlags, nextInheritedFlags))
                }
            }

            if (needsIfWrapper) {
                tab(ifClause) {
                    app(" {")
                    app("", readCode)
                }
                tab("}")
            } else {
                app("", readCode)
            }
        }
    }

    fun condition(expr : Expr) : String? {
        if (expr.canBeEvaluatedToAVariable() && !expr.isVariable()) {
            // create an if case for all dependencies that might be null
            val nullables = expr.dependencies.filter {
                it.isMandatory && it.other.resolvedType.isNullable
            }.map { it.other }
            if (!expr.isEqualityCheck && nullables.isNotEmpty()) {
                return "${nullables.map { "${it.executePendingLocalName} != null" }.joinToString(" && ")}"
            } else {
                return null
            }
        } else {
            return null
        }
    }

    fun declareListenerImpls() = kcode("// Listener Stub Implementations") {
        model.exprMap.values.filter {
            it.isUsed && it is ListenerExpr
        }.groupBy { it }.forEach {
            val expr = it.key as ListenerExpr
            val listenerType = expr.resolvedType;
            val extendsImplements : String
            if (listenerType.isInterface) {
                extendsImplements = "implements"
            } else {
                extendsImplements = "extends"
            }
            nl("public static class ${expr.listenerClassName} $extendsImplements ${listenerType.canonicalName}{") {
                if (expr.child.isDynamic) {
                    tab("private ${expr.child.resolvedType.toJavaCode()} value;")
                    tab("public ${expr.listenerClassName} setValue(${expr.child.resolvedType.toJavaCode()} value) {") {
                        tab("this.value = value;")
                        tab("return value == null ? null : this;")
                    }
                    tab("}")
                }
                val listenerMethod = expr.method
                val parameterTypes = listenerMethod.parameterTypes
                val returnType = listenerMethod.getReturnType(parameterTypes.toArrayList())
                tab("@Override")
                tab("public $returnType ${listenerMethod.name}(${
                    parameterTypes.withIndex().map {
                        "${it.value.toJavaCode()} arg${it.index}"
                    }.joinToString(", ")
                }) {") {
                    val obj : String
                    if (expr.child.isDynamic) {
                        obj = "this.value"
                    } else {
                        obj = expr.child.toCode().generate();
                    }
                    val returnStr : String
                    if (!returnType.isVoid) {
                        returnStr = "return "
                    } else {
                        returnStr = ""
                    }
                    val args = parameterTypes.withIndex().map {
                        "arg${it.index}"
                    }.joinToString(", ")
                    tab("$returnStr$obj.${expr.name}($args);")
                }
                tab("}")
            }
            nl("}")
        }
    }

    fun declareFactories() = kcode("") {
        nl("public static $baseClassName inflate(android.view.LayoutInflater inflater, android.view.ViewGroup root, boolean attachToRoot) {") {
            tab("return inflate(inflater, root, attachToRoot, android.databinding.DataBindingUtil.getDefaultComponent());")
        }
        nl("}")
        nl("public static $baseClassName inflate(android.view.LayoutInflater inflater, android.view.ViewGroup root, boolean attachToRoot, android.databinding.DataBindingComponent bindingComponent) {") {
            tab("return android.databinding.DataBindingUtil.<$baseClassName>inflate(inflater, ${layoutBinder.modulePackage}.R.layout.${layoutBinder.layoutname}, root, attachToRoot, bindingComponent);")
        }
        nl("}")
        if (!layoutBinder.isMerge) {
            nl("public static $baseClassName inflate(android.view.LayoutInflater inflater) {") {
                tab("return inflate(inflater, android.databinding.DataBindingUtil.getDefaultComponent());")
            }
            nl("}")
            nl("public static $baseClassName inflate(android.view.LayoutInflater inflater, android.databinding.DataBindingComponent bindingComponent) {") {
                tab("return bind(inflater.inflate(${layoutBinder.modulePackage}.R.layout.${layoutBinder.layoutname}, null, false), bindingComponent);")
            }
            nl("}")
            nl("public static $baseClassName bind(android.view.View view) {") {
                tab("return bind(view, android.databinding.DataBindingUtil.getDefaultComponent());")
            }
            nl("}")
            nl("public static $baseClassName bind(android.view.View view, android.databinding.DataBindingComponent bindingComponent) {") {
                tab("if (!\"${layoutBinder.tag}_0\".equals(view.getTag())) {") {
                    tab("throw new RuntimeException(\"view tag isn't correct on view:\" + view.getTag());")
                }
                tab("}")
                tab("return new $baseClassName(bindingComponent, view);")
            }
            nl("}")
        }
    }

    /**
     * When called for a library compilation, we do not generate real implementations
     */
    public fun writeBaseClass(forLibrary : Boolean) : String =
        kcode("package ${layoutBinder.`package`};") {
            nl("import android.databinding.Bindable;")
            nl("import android.databinding.DataBindingUtil;")
            nl("import android.databinding.ViewDataBinding;")
            nl("public abstract class $baseClassName extends ViewDataBinding {")
            layoutBinder.sortedTargets.filter{it.id != null}.forEach {
                tab("public final ${it.interfaceClass} ${it.fieldName};")
            }
            nl("")
            tab("protected $baseClassName(android.databinding.DataBindingComponent bindingComponent, android.view.View root_, int localFieldCount") {
                layoutBinder.sortedTargets.filter{it.id != null}.forEach {
                    tab(", ${it.interfaceClass} ${it.constructorParamName}")
                }
            }
            tab(") {") {
                tab("super(bindingComponent, root_, localFieldCount);")
                layoutBinder.sortedTargets.filter{it.id != null}.forEach {
                    tab("this.${it.fieldName} = ${it.constructorParamName};")
                }
            }
            tab("}")
            nl("")
            variables.forEach {
                if (it.userDefinedType != null) {
                    val type = ModelAnalyzer.getInstance().applyImports(it.userDefinedType, model.imports)
                    tab("public abstract void ${it.setterName}($type ${it.readableName});")
                }
            }
            tab("public static $baseClassName inflate(android.view.LayoutInflater inflater, android.view.ViewGroup root, boolean attachToRoot) {") {
                tab("return inflate(inflater, root, attachToRoot, android.databinding.DataBindingUtil.getDefaultComponent());")
            }
            tab("}")
            tab("public static $baseClassName inflate(android.view.LayoutInflater inflater) {") {
                tab("return inflate(inflater, android.databinding.DataBindingUtil.getDefaultComponent());")
            }
            tab("}")
            tab("public static $baseClassName bind(android.view.View view) {") {
                if (forLibrary) {
                    tab("return null;")
                } else {
                    tab("return bind(view, android.databinding.DataBindingUtil.getDefaultComponent());")
                }
            }
            tab("}")
            tab("public static $baseClassName inflate(android.view.LayoutInflater inflater, android.view.ViewGroup root, boolean attachToRoot, android.databinding.DataBindingComponent bindingComponent) {") {
                if (forLibrary) {
                    tab("return null;")
                } else {
                    tab("return DataBindingUtil.<$baseClassName>inflate(inflater, ${layoutBinder.modulePackage}.R.layout.${layoutBinder.layoutname}, root, attachToRoot, bindingComponent);")
                }
            }
            tab("}")
            tab("public static $baseClassName inflate(android.view.LayoutInflater inflater, android.databinding.DataBindingComponent bindingComponent) {") {
                if (forLibrary) {
                    tab("return null;")
                } else {
                    tab("return DataBindingUtil.<$baseClassName>inflate(inflater, ${layoutBinder.modulePackage}.R.layout.${layoutBinder.layoutname}, null, false, bindingComponent);")
                }
            }
            tab("}")
            tab("public static $baseClassName bind(android.view.View view, android.databinding.DataBindingComponent bindingComponent) {") {
                if (forLibrary) {
                    tab("return null;")
                } else {
                    tab("return ($baseClassName)bind(bindingComponent, view, ${layoutBinder.modulePackage}.R.layout.${layoutBinder.layoutname});")
                }
            }
            tab("}")
            nl("}")
        }.generate()
}
