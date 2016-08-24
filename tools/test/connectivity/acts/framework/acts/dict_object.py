#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.


class DictObject(dict):
    """Optional convenient base type for creating simple objects that are
    naturally serializable.

    A DictObject provides object-oriented access semantics to a dictionary,
    allowing it to look like a class with defined members. By ensuring that
    all of the class members are serializable, the object can be serialized
    as a dictionary/de-serialized from a dictionary.
    """

    def __init__(self, *args, **kwargs):
        """Constructor for a dictionary-as-object representation of kwargs

        Args:
            args: Currently unused - included for completeness
            kwargs: keyword arguments used to construct the underlying dict

        Returns:
            Instance of DictObject
        """
        super(DictObject, self).update(**kwargs)

    def __getattr__(self, name):
        """Returns a key from the superclass dictionary as an attribute

        Args:
            name: name of the pseudo class attribute

        Returns:
            Dictionary item stored at "name"

        Raises:
            AttributeError if the item is not found
        """
        try:
            return self[name]
        except KeyError as ke:
            raise AttributeError(ke)

    def __setattr__(self, name, value):
        """Updates the value of a key=name to a given value

        Args:
            name: name of the pseudo class attribute
            value: value of the key

        Raises:
            AttributeError if the item is not found
        """
        if name in super(DictObject, self).keys():
            super(DictObject, self).__setitem__(name, value)
        else:
            raise AttributeError("Class does not have attribute {}"
                                 .format(value))

    @classmethod
    def from_dict(cls, dictionary):
        """Factory method for constructing a DictObject from a dictionary

        Args:
            dictionary: Dictionary used to construct the DictObject

        Returns:
            Instance of DictObject
        """
        c = cls()
        c.update(dictionary)
        return c
