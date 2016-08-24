# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Model extensions common to both the server and client rdb modules.
"""


from django.core import exceptions as django_exceptions
from django.db import models as dbmodels


from autotest_lib.client.common_lib import host_protections
from autotest_lib.client.common_lib import host_states
from autotest_lib.frontend import settings


class ModelValidators(object):
    """Convenience functions for model validation.

    This model is duplicated both on  the client and server rdb. Any method
    added to this class must only be capable of class level validation of model
    fields, since anything else is meaningless on the client side.
    """
    # TODO: at least some of these functions really belong in a custom
    # Manager class.

    field_dict = None
    # subclasses should override if they want to support smart_get() by name
    name_field = None

    @classmethod
    def get_field_dict(cls):
        if cls.field_dict is None:
            cls.field_dict = {}
            for field in cls._meta.fields:
                cls.field_dict[field.name] = field
        return cls.field_dict


    @classmethod
    def clean_foreign_keys(cls, data):
        """\
        -Convert foreign key fields in data from <field>_id to just
        <field>.
        -replace foreign key objects with their IDs
        This method modifies data in-place.
        """
        for field in cls._meta.fields:
            if not field.rel:
                continue
            if (field.attname != field.name and
                field.attname in data):
                data[field.name] = data[field.attname]
                del data[field.attname]
            if field.name not in data:
                continue
            value = data[field.name]
            if isinstance(value, dbmodels.Model):
                data[field.name] = value._get_pk_val()


    @classmethod
    def _convert_booleans(cls, data):
        """
        Ensure BooleanFields actually get bool values.  The Django MySQL
        backend returns ints for BooleanFields, which is almost always not
        a problem, but it can be annoying in certain situations.
        """
        for field in cls._meta.fields:
            if type(field) == dbmodels.BooleanField and field.name in data:
                data[field.name] = bool(data[field.name])


    # TODO(showard) - is there a way to not have to do this?
    @classmethod
    def provide_default_values(cls, data):
        """\
        Provide default values for fields with default values which have
        nothing passed in.

        For CharField and TextField fields with "blank=True", if nothing
        is passed, we fill in an empty string value, even if there's no
        :retab default set.
        """
        new_data = dict(data)
        field_dict = cls.get_field_dict()
        for name, obj in field_dict.iteritems():
            if data.get(name) is not None:
                continue
            if obj.default is not dbmodels.fields.NOT_PROVIDED:
                new_data[name] = obj.default
            elif (isinstance(obj, dbmodels.CharField) or
                  isinstance(obj, dbmodels.TextField)):
                new_data[name] = ''
        return new_data


    @classmethod
    def validate_field_names(cls, data):
        'Checks for extraneous fields in data.'
        errors = {}
        field_dict = cls.get_field_dict()
        for field_name in data:
            if field_name not in field_dict:
                errors[field_name] = 'No field of this name'
        return errors


    @classmethod
    def prepare_data_args(cls, data):
        'Common preparation for add_object and update_object'
        # must check for extraneous field names here, while we have the
        # data in a dict
        errors = cls.validate_field_names(data)
        if errors:
            raise django_exceptions.ValidationError(errors)
        return data


    @classmethod
    def _get_required_field_names(cls):
        """Get the fields without which we cannot create a host.

        @return: A list of field names that cannot be blank on host creation.
        """
        return [field.name for field in cls._meta.fields if not field.blank]


    @classmethod
    def get_basic_field_names(cls):
        """Get all basic fields of the Model.

        This method returns the names of all fields that the client can provide
        a value for during host creation. The fields not included in this list
        are those that we can leave blank, such as synch_id. Specifying non
        null values for such fields only makes sense as an update to the host.

        @return A list of basic fields.
            Eg: set([hostname, locked, leased, synch_id, status, invalid,
                     protection, lock_time, dirty])
        """
        return [field.name for field in cls._meta.fields
                if field.has_default()] + cls._get_required_field_names()


    @classmethod
    def validate_model_fields(cls, data):
        """Validate parameters needed to create a host.

        Check that all required fields are specified, that specified fields
        are actual model values, and provide defaults for the unspecified
        but unrequired fields.

        @param dict: A dictionary with the args to create the model.

        @raises dajngo_exceptions.ValidationError: If either an invalid field
            is specified or a required field is missing.
        """
        missing_fields = set(cls._get_required_field_names()) - set(data.keys())
        if missing_fields:
            raise django_exceptions.ValidationError('%s required to create %s, '
                    'supplied %s ' % (missing_fields, cls.__name__, data))
        data = cls.prepare_data_args(data)
        data = cls.provide_default_values(data)
        return data


class AbstractHostModel(dbmodels.Model, ModelValidators):
    """Abstract model specifying all fields one can use to create a host.

    This model enforces consistency between the host models of the rdb and
    their representation on the client side.

    Internal fields:
        synch_id: currently unused
        status: string describing status of host
        invalid: true if the host has been deleted
        protection: indicates what can be done to this host during repair
        lock_time: DateTime at which the host was locked
        dirty: true if the host has been used without being rebooted
        lock_reason: The reason for locking the host.
    """
    Status = host_states.Status
    hostname = dbmodels.CharField(max_length=255, unique=True)
    locked = dbmodels.BooleanField(default=False)
    leased = dbmodels.BooleanField(default=True)
    synch_id = dbmodels.IntegerField(blank=True, null=True,
                                     editable=settings.FULL_ADMIN)
    status = dbmodels.CharField(max_length=255, default=Status.READY,
                                choices=Status.choices(),
                                editable=settings.FULL_ADMIN)
    invalid = dbmodels.BooleanField(default=False,
                                    editable=settings.FULL_ADMIN)
    protection = dbmodels.SmallIntegerField(null=False, blank=True,
                                            choices=host_protections.choices,
                                            default=host_protections.default)
    lock_time = dbmodels.DateTimeField(null=True, blank=True, editable=False)
    dirty = dbmodels.BooleanField(default=True, editable=settings.FULL_ADMIN)
    lock_reason = dbmodels.CharField(null=True, max_length=255, blank=True,
                                     default='')


    class Meta:
        abstract = True
