package jdiff;

import java.io.*;
import java.util.*;

/**
 * Class to represent a constructor, analogous to ConstructorDoc in the
 * Javadoc doclet API.
 *
 * The method used for Collection comparison (compareTo) must make its
 * comparison based upon everything that is known about this constructor.
 *
 * See the file LICENSE.txt for copyright details.
 * @author Matthew Doar, mdoar@pobox.com
 */
class ConstructorAPI implements Comparable {
    /**
     * Name of the constructor.
     * Either this or type_ must be non-null
     */
    public String name_ = null;

    /**
     * The type of the constructor, being all the parameter types
     * separated by commas.
     * Either this or name_ must be non-null.
     */
    public String type_ = null;

    /**
     * The exceptions thrown by this constructor, being all the exception types
     * separated by commas. "no exceptions" if no exceptions are thrown.
     */
    public String exceptions_ = "no exceptions";

    /** Modifiers for this class. */
    public Modifiers modifiers_;

    public List params_; // ParamAPI[]

    /** The doc block, default is null. */
    public String doc_ = null;

    /** Constructor. */
    public ConstructorAPI(String name, String type, Modifiers modifiers) {
        if (name == null && type == null) {
            throw new IllegalArgumentException("Cannot have constructor with both name and type"
                + "being null");
        }
        name_ = name;
        type_ = type;
        modifiers_ = modifiers;
        params_ = new ArrayList();
    }

    private static <T extends Comparable<? super T>> int compareNullIsLeast(T c1, T c2) {
        return c1 == null ? (c2 == null ? 0 : -1) : (c2 == null ? 1 : c1.compareTo(c2));
    }

    /** Compare two ConstructorAPI objects by type and modifiers. */
    public int compareTo(Object o) {
        ConstructorAPI constructorAPI = (ConstructorAPI)o;
        int comp = compareNullIsLeast(name_, constructorAPI.name_);
        if (comp != 0)
            return comp;
        comp = compareNullIsLeast(getSignature(), constructorAPI.getSignature());
        if (comp != 0)
            return comp;
        comp = exceptions_.compareTo(constructorAPI.exceptions_);
        if (comp != 0)
            return comp;
        comp = modifiers_.compareTo(constructorAPI.modifiers_);
        if (comp != 0)
            return comp;
        if (APIComparator.docChanged(doc_, constructorAPI.doc_))
            return -1;
        return 0;
    }

    /**
     * Tests two constructors, using just the name and type, used by indexOf().
     */
    public boolean equals(Object o) {
        ConstructorAPI constructorAPI = (ConstructorAPI)o;
        if (compareNullIsLeast(name_, constructorAPI.name_) == 0 &&
                compareNullIsLeast(type_, constructorAPI.type_) == 0)
            return true;
        return false;
    }

    /**
     * Tests two methods for equality, using just the signature.
     */
    public boolean equalSignatures(Object o) {
        if (getSignature().compareTo(((MethodAPI)o).getSignature()) == 0)
            return true;
        return false;
    }

    /** Cached result of getSignature(). */
    private String signature_ = null;

    /** Return the signature of the method. */
    public String getSignature() {
        if (signature_ != null)
            return signature_;
        if (params_ == null)
            return type_;
        String res = "";
        boolean first = true;
        Iterator iter = params_.iterator();
        while (iter.hasNext()) {
            if (!first)
                res += ", ";
            ParamAPI param = (ParamAPI)(iter.next());
            res += param.toString();
            first = false;
        }
        signature_ = res;
        return res;
    }
}
