// Generated source.
package org.hamcrest;

public class Matchers {

    /**
     * Decorates another Matcher, retaining the behavior but allowing tests
     * to be slightly more expressive.
     * 
     * eg. assertThat(cheese, equalTo(smelly))
     * vs assertThat(cheese, is(equalTo(smelly)))
     */
    public static <T> org.hamcrest.Matcher<T> is(org.hamcrest.Matcher<T> matcher) {
        return org.hamcrest.core.Is.is(matcher);
    }

    /**
     * This is a shortcut to the frequently used is(equalTo(x)).
     * 
     * eg. assertThat(cheese, is(equalTo(smelly)))
     * vs assertThat(cheese, is(smelly))
     */
    public static <T> org.hamcrest.Matcher<T> is(T value) {
        return org.hamcrest.core.Is.is(value);
    }

    /**
     * This is a shortcut to the frequently used is(instanceOf(SomeClass.class)).
     * 
     * eg. assertThat(cheese, is(instanceOf(Cheddar.class)))
     * vs assertThat(cheese, is(Cheddar.class))
     */
    public static org.hamcrest.Matcher<java.lang.Object> is(java.lang.Class<?> type) {
        return org.hamcrest.core.Is.is(type);
    }

    /**
     * Inverts the rule.
     */
    public static <T> org.hamcrest.Matcher<T> not(org.hamcrest.Matcher<T> matcher) {
        return org.hamcrest.core.IsNot.not(matcher);
    }

    /**
     * This is a shortcut to the frequently used not(equalTo(x)).
     * 
     * eg. assertThat(cheese, is(not(equalTo(smelly))))
     * vs assertThat(cheese, is(not(smelly)))
     */
    public static <T> org.hamcrest.Matcher<T> not(T value) {
        return org.hamcrest.core.IsNot.not(value);
    }

    /**
     * Is the value equal to another value, as tested by the
     * {@link java.lang.Object#equals} invokedMethod?
     */
    public static <T> org.hamcrest.Matcher<T> equalTo(T operand) {
        return org.hamcrest.core.IsEqual.equalTo(operand);
    }

    /**
     * Is the value an instance of a particular type?
     */
    public static org.hamcrest.Matcher<java.lang.Object> instanceOf(java.lang.Class<?> type) {
        return org.hamcrest.core.IsInstanceOf.instanceOf(type);
    }

    /**
     * Evaluates to true only if ALL of the passed in matchers evaluate to true.
     */
    public static <T> org.hamcrest.Matcher<T> allOf(org.hamcrest.Matcher<? extends T>... matchers) {
        return org.hamcrest.core.AllOf.allOf(matchers);
    }

    /**
     * Evaluates to true only if ALL of the passed in matchers evaluate to true.
     */
    public static <T> org.hamcrest.Matcher<T> allOf(java.lang.Iterable<org.hamcrest.Matcher<? extends T>> matchers) {
        return org.hamcrest.core.AllOf.allOf(matchers);
    }

    /**
     * Evaluates to true if ANY of the passed in matchers evaluate to true.
     */
    public static <T> org.hamcrest.Matcher<T> anyOf(org.hamcrest.Matcher<? extends T>... matchers) {
        return org.hamcrest.core.AnyOf.anyOf(matchers);
    }

    /**
     * Evaluates to true if ANY of the passed in matchers evaluate to true.
     */
    public static <T> org.hamcrest.Matcher<T> anyOf(java.lang.Iterable<org.hamcrest.Matcher<? extends T>> matchers) {
        return org.hamcrest.core.AnyOf.anyOf(matchers);
    }

    /**
     * Creates a new instance of IsSame
     * 
     * @param object The predicate evaluates to true only when the argument is
     * this object.
     */
    public static <T> org.hamcrest.Matcher<T> sameInstance(T object) {
        return org.hamcrest.core.IsSame.sameInstance(object);
    }

    /**
     * This matcher always evaluates to true.
     */
    public static <T> org.hamcrest.Matcher<T> anything() {
        return org.hamcrest.core.IsAnything.anything();
    }

    /**
     * This matcher always evaluates to true.
     * 
     * @param description A meaningful string used when describing itself.
     */
    public static <T> org.hamcrest.Matcher<T> anything(java.lang.String description) {
        return org.hamcrest.core.IsAnything.anything(description);
    }

    /**
     * This matcher always evaluates to true. With type inference.
     */
    public static <T> org.hamcrest.Matcher<T> any(java.lang.Class<T> type) {
        return org.hamcrest.core.IsAnything.any(type);
    }

    /**
     * Matches if value is null.
     */
    public static <T> org.hamcrest.Matcher<T> nullValue() {
        return org.hamcrest.core.IsNull.nullValue();
    }

    /**
     * Matches if value is null. With type inference.
     */
    public static <T> org.hamcrest.Matcher<T> nullValue(java.lang.Class<T> type) {
        return org.hamcrest.core.IsNull.nullValue(type);
    }

    /**
     * Matches if value is not null.
     */
    public static <T> org.hamcrest.Matcher<T> notNullValue() {
        return org.hamcrest.core.IsNull.notNullValue();
    }

    /**
     * Matches if value is not null. With type inference.
     */
    public static <T> org.hamcrest.Matcher<T> notNullValue(java.lang.Class<T> type) {
        return org.hamcrest.core.IsNull.notNullValue(type);
    }

    /**
     * Wraps an existing matcher and overrides the description when it fails.
     */
    public static <T> org.hamcrest.Matcher<T> describedAs(java.lang.String description, org.hamcrest.Matcher<T> matcher, java.lang.Object... values) {
        return org.hamcrest.core.DescribedAs.describedAs(description, matcher, values);
    }

    public static <T> org.hamcrest.Matcher<T[]> hasItemInArray(org.hamcrest.Matcher<T> elementMatcher) {
        return org.hamcrest.collection.IsArrayContaining.hasItemInArray(elementMatcher);
    }

    public static <T> org.hamcrest.Matcher<T[]> hasItemInArray(T element) {
        return org.hamcrest.collection.IsArrayContaining.hasItemInArray(element);
    }

    public static <T> org.hamcrest.Matcher<java.lang.Iterable<T>> hasItem(T element) {
        return org.hamcrest.collection.IsCollectionContaining.hasItem(element);
    }

    public static <T> org.hamcrest.Matcher<java.lang.Iterable<T>> hasItem(org.hamcrest.Matcher<? extends T> elementMatcher) {
        return org.hamcrest.collection.IsCollectionContaining.hasItem(elementMatcher);
    }

    public static <T> org.hamcrest.Matcher<java.lang.Iterable<T>> hasItems(org.hamcrest.Matcher<? extends T>... elementMatchers) {
        return org.hamcrest.collection.IsCollectionContaining.hasItems(elementMatchers);
    }

    public static <T> org.hamcrest.Matcher<java.lang.Iterable<T>> hasItems(T... elements) {
        return org.hamcrest.collection.IsCollectionContaining.hasItems(elements);
    }

    public static <K, V> org.hamcrest.Matcher<java.util.Map<K, V>> hasEntry(org.hamcrest.Matcher<K> keyMatcher, org.hamcrest.Matcher<V> valueMatcher) {
        return org.hamcrest.collection.IsMapContaining.hasEntry(keyMatcher, valueMatcher);
    }

    public static <K, V> org.hamcrest.Matcher<java.util.Map<K, V>> hasEntry(K key, V value) {
        return org.hamcrest.collection.IsMapContaining.hasEntry(key, value);
    }

    public static <K, V> org.hamcrest.Matcher<java.util.Map<K, V>> hasKey(org.hamcrest.Matcher<K> keyMatcher) {
        return org.hamcrest.collection.IsMapContaining.hasKey(keyMatcher);
    }

    public static <K, V> org.hamcrest.Matcher<java.util.Map<K, V>> hasKey(K key) {
        return org.hamcrest.collection.IsMapContaining.hasKey(key);
    }

    public static <K, V> org.hamcrest.Matcher<java.util.Map<K, V>> hasValue(org.hamcrest.Matcher<V> valueMatcher) {
        return org.hamcrest.collection.IsMapContaining.hasValue(valueMatcher);
    }

    public static <K, V> org.hamcrest.Matcher<java.util.Map<K, V>> hasValue(V value) {
        return org.hamcrest.collection.IsMapContaining.hasValue(value);
    }

    public static <T> org.hamcrest.Matcher<T> isIn(java.util.Collection<T> collection) {
        return org.hamcrest.collection.IsIn.isIn(collection);
    }

    public static <T> org.hamcrest.Matcher<T> isIn(T[] param1) {
        return org.hamcrest.collection.IsIn.isIn(param1);
    }

    public static <T> org.hamcrest.Matcher<T> isOneOf(T... elements) {
        return org.hamcrest.collection.IsIn.isOneOf(elements);
    }

    public static org.hamcrest.Matcher<java.lang.Double> closeTo(double operand, double error) {
        return org.hamcrest.number.IsCloseTo.closeTo(operand, error);
    }

    public static <T extends java.lang.Comparable<T>> org.hamcrest.Matcher<T> greaterThan(T value) {
        return org.hamcrest.number.OrderingComparisons.greaterThan(value);
    }

    public static <T extends java.lang.Comparable<T>> org.hamcrest.Matcher<T> greaterThanOrEqualTo(T value) {
        return org.hamcrest.number.OrderingComparisons.greaterThanOrEqualTo(value);
    }

    public static <T extends java.lang.Comparable<T>> org.hamcrest.Matcher<T> lessThan(T value) {
        return org.hamcrest.number.OrderingComparisons.lessThan(value);
    }

    public static <T extends java.lang.Comparable<T>> org.hamcrest.Matcher<T> lessThanOrEqualTo(T value) {
        return org.hamcrest.number.OrderingComparisons.lessThanOrEqualTo(value);
    }

    public static org.hamcrest.Matcher<java.lang.String> equalToIgnoringCase(java.lang.String string) {
        return org.hamcrest.text.IsEqualIgnoringCase.equalToIgnoringCase(string);
    }

    public static org.hamcrest.Matcher<java.lang.String> equalToIgnoringWhiteSpace(java.lang.String string) {
        return org.hamcrest.text.IsEqualIgnoringWhiteSpace.equalToIgnoringWhiteSpace(string);
    }

    public static org.hamcrest.Matcher<java.lang.String> containsString(java.lang.String substring) {
        return org.hamcrest.text.StringContains.containsString(substring);
    }

    public static org.hamcrest.Matcher<java.lang.String> endsWith(java.lang.String substring) {
        return org.hamcrest.text.StringEndsWith.endsWith(substring);
    }

    public static org.hamcrest.Matcher<java.lang.String> startsWith(java.lang.String substring) {
        return org.hamcrest.text.StringStartsWith.startsWith(substring);
    }

    public static <T> org.hamcrest.Matcher<T> hasToString(org.hamcrest.Matcher<java.lang.String> toStringMatcher) {
        return org.hamcrest.object.HasToString.hasToString(toStringMatcher);
    }

    public static <T> org.hamcrest.Matcher<java.lang.Class<?>> typeCompatibleWith(java.lang.Class<T> baseType) {
        return org.hamcrest.object.IsCompatibleType.typeCompatibleWith(baseType);
    }

    /**
     * Constructs an IsEventFrom Matcher that returns true for any object
     * derived from <var>eventClass</var> announced by <var>source</var>.
     */
    public static org.hamcrest.Matcher<java.util.EventObject> eventFrom(java.lang.Class<? extends java.util.EventObject> eventClass, java.lang.Object source) {
        return org.hamcrest.object.IsEventFrom.eventFrom(eventClass, source);
    }

    /**
     * Constructs an IsEventFrom Matcher that returns true for any object
     * derived from {@link java.util.EventObject} announced by <var>source
     * </var>.
     */
    public static org.hamcrest.Matcher<java.util.EventObject> eventFrom(java.lang.Object source) {
        return org.hamcrest.object.IsEventFrom.eventFrom(source);
    }

    /* android-changed REMOVE
  public static <T> org.hamcrest.Matcher<T> hasProperty(java.lang.String propertyName) {
    return org.hamcrest.beans.HasProperty.hasProperty(propertyName);
  }

  public static <T> org.hamcrest.Matcher<T> hasProperty(java.lang.String propertyName, org.hamcrest.Matcher value) {
    return org.hamcrest.beans.HasPropertyWithValue.hasProperty(propertyName, value);
  }
     */

    public static org.hamcrest.Matcher<org.w3c.dom.Node> hasXPath(java.lang.String xPath, org.hamcrest.Matcher<java.lang.String> valueMatcher) {
        return org.hamcrest.xml.HasXPath.hasXPath(xPath, valueMatcher);
    }

    public static org.hamcrest.Matcher<org.w3c.dom.Node> hasXPath(java.lang.String xPath) {
        return org.hamcrest.xml.HasXPath.hasXPath(xPath);
    }

}
