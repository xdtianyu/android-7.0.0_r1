"""
Extensions to Django's model logic.
"""

import django.core.exceptions
from django.db import backend
from django.db import connection
from django.db import connections
from django.db import models as dbmodels
from django.db import transaction
from django.db.models.sql import query
import django.db.models.sql.where
from django.utils import datastructures
from autotest_lib.client.common_lib.cros.graphite import autotest_stats
from autotest_lib.frontend.afe import rdb_model_extensions


class ValidationError(django.core.exceptions.ValidationError):
    """\
    Data validation error in adding or updating an object. The associated
    value is a dictionary mapping field names to error strings.
    """

def _quote_name(name):
    """Shorthand for connection.ops.quote_name()."""
    return connection.ops.quote_name(name)


class LeasedHostManager(dbmodels.Manager):
    """Query manager for unleased, unlocked hosts.
    """
    def get_query_set(self):
        return (super(LeasedHostManager, self).get_query_set().filter(
                leased=0, locked=0))


class ExtendedManager(dbmodels.Manager):
    """\
    Extended manager supporting subquery filtering.
    """

    class CustomQuery(query.Query):
        def __init__(self, *args, **kwargs):
            super(ExtendedManager.CustomQuery, self).__init__(*args, **kwargs)
            self._custom_joins = []


        def clone(self, klass=None, **kwargs):
            obj = super(ExtendedManager.CustomQuery, self).clone(klass)
            obj._custom_joins = list(self._custom_joins)
            return obj


        def combine(self, rhs, connector):
            super(ExtendedManager.CustomQuery, self).combine(rhs, connector)
            if hasattr(rhs, '_custom_joins'):
                self._custom_joins.extend(rhs._custom_joins)


        def add_custom_join(self, table, condition, join_type,
                            condition_values=(), alias=None):
            if alias is None:
                alias = table
            join_dict = dict(table=table,
                             condition=condition,
                             condition_values=condition_values,
                             join_type=join_type,
                             alias=alias)
            self._custom_joins.append(join_dict)


        @classmethod
        def convert_query(self, query_set):
            """
            Convert the query set's "query" attribute to a CustomQuery.
            """
            # Make a copy of the query set
            query_set = query_set.all()
            query_set.query = query_set.query.clone(
                    klass=ExtendedManager.CustomQuery,
                    _custom_joins=[])
            return query_set


    class _WhereClause(object):
        """Object allowing us to inject arbitrary SQL into Django queries.

        By using this instead of extra(where=...), we can still freely combine
        queries with & and |.
        """
        def __init__(self, clause, values=()):
            self._clause = clause
            self._values = values


        def as_sql(self, qn=None, connection=None):
            return self._clause, self._values


        def relabel_aliases(self, change_map):
            return


    def add_join(self, query_set, join_table, join_key, join_condition='',
                 join_condition_values=(), join_from_key=None, alias=None,
                 suffix='', exclude=False, force_left_join=False):
        """Add a join to query_set.

        Join looks like this:
                (INNER|LEFT) JOIN <join_table> AS <alias>
                    ON (<this table>.<join_from_key> = <join_table>.<join_key>
                        and <join_condition>)

        @param join_table table to join to
        @param join_key field referencing back to this model to use for the join
        @param join_condition extra condition for the ON clause of the join
        @param join_condition_values values to substitute into join_condition
        @param join_from_key column on this model to join from.
        @param alias alias to use for for join
        @param suffix suffix to add to join_table for the join alias, if no
                alias is provided
        @param exclude if true, exclude rows that match this join (will use a
        LEFT OUTER JOIN and an appropriate WHERE condition)
        @param force_left_join - if true, a LEFT OUTER JOIN will be used
        instead of an INNER JOIN regardless of other options
        """
        join_from_table = query_set.model._meta.db_table
        if join_from_key is None:
            join_from_key = self.model._meta.pk.name
        if alias is None:
            alias = join_table + suffix
        full_join_key = _quote_name(alias) + '.' + _quote_name(join_key)
        full_join_condition = '%s = %s.%s' % (full_join_key,
                                              _quote_name(join_from_table),
                                              _quote_name(join_from_key))
        if join_condition:
            full_join_condition += ' AND (' + join_condition + ')'
        if exclude or force_left_join:
            join_type = query_set.query.LOUTER
        else:
            join_type = query_set.query.INNER

        query_set = self.CustomQuery.convert_query(query_set)
        query_set.query.add_custom_join(join_table,
                                        full_join_condition,
                                        join_type,
                                        condition_values=join_condition_values,
                                        alias=alias)

        if exclude:
            query_set = query_set.extra(where=[full_join_key + ' IS NULL'])

        return query_set


    def _info_for_many_to_one_join(self, field, join_to_query, alias):
        """
        @param field: the ForeignKey field on the related model
        @param join_to_query: the query over the related model that we're
                joining to
        @param alias: alias of joined table
        """
        info = {}
        rhs_table = join_to_query.model._meta.db_table
        info['rhs_table'] = rhs_table
        info['rhs_column'] = field.column
        info['lhs_column'] = field.rel.get_related_field().column
        rhs_where = join_to_query.query.where
        rhs_where.relabel_aliases({rhs_table: alias})
        compiler = join_to_query.query.get_compiler(using=join_to_query.db)
        initial_clause, values = compiler.as_sql()
        all_clauses = (initial_clause,)
        if hasattr(join_to_query.query, 'extra_where'):
            all_clauses += join_to_query.query.extra_where
        info['where_clause'] = (
                    ' AND '.join('(%s)' % clause for clause in all_clauses))
        info['values'] = values
        return info


    def _info_for_many_to_many_join(self, m2m_field, join_to_query, alias,
                                    m2m_is_on_this_model):
        """
        @param m2m_field: a Django field representing the M2M relationship.
                It uses a pivot table with the following structure:
                this model table <---> M2M pivot table <---> joined model table
        @param join_to_query: the query over the related model that we're
                joining to.
        @param alias: alias of joined table
        """
        if m2m_is_on_this_model:
            # referenced field on this model
            lhs_id_field = self.model._meta.pk
            # foreign key on the pivot table referencing lhs_id_field
            m2m_lhs_column = m2m_field.m2m_column_name()
            # foreign key on the pivot table referencing rhd_id_field
            m2m_rhs_column = m2m_field.m2m_reverse_name()
            # referenced field on related model
            rhs_id_field = m2m_field.rel.get_related_field()
        else:
            lhs_id_field = m2m_field.rel.get_related_field()
            m2m_lhs_column = m2m_field.m2m_reverse_name()
            m2m_rhs_column = m2m_field.m2m_column_name()
            rhs_id_field = join_to_query.model._meta.pk

        info = {}
        info['rhs_table'] = m2m_field.m2m_db_table()
        info['rhs_column'] = m2m_lhs_column
        info['lhs_column'] = lhs_id_field.column

        # select the ID of related models relevant to this join.  we can only do
        # a single join, so we need to gather this information up front and
        # include it in the join condition.
        rhs_ids = join_to_query.values_list(rhs_id_field.attname, flat=True)
        assert len(rhs_ids) == 1, ('Many-to-many custom field joins can only '
                                   'match a single related object.')
        rhs_id = rhs_ids[0]

        info['where_clause'] = '%s.%s = %s' % (_quote_name(alias),
                                               _quote_name(m2m_rhs_column),
                                               rhs_id)
        info['values'] = ()
        return info


    def join_custom_field(self, query_set, join_to_query, alias,
                          left_join=True):
        """Join to a related model to create a custom field in the given query.

        This method is used to construct a custom field on the given query based
        on a many-valued relationsip.  join_to_query should be a simple query
        (no joins) on the related model which returns at most one related row
        per instance of this model.

        For many-to-one relationships, the joined table contains the matching
        row from the related model it one is related, NULL otherwise.

        For many-to-many relationships, the joined table contains the matching
        row if it's related, NULL otherwise.
        """
        relationship_type, field = self.determine_relationship(
                join_to_query.model)

        if relationship_type == self.MANY_TO_ONE:
            info = self._info_for_many_to_one_join(field, join_to_query, alias)
        elif relationship_type == self.M2M_ON_RELATED_MODEL:
            info = self._info_for_many_to_many_join(
                    m2m_field=field, join_to_query=join_to_query, alias=alias,
                    m2m_is_on_this_model=False)
        elif relationship_type ==self.M2M_ON_THIS_MODEL:
            info = self._info_for_many_to_many_join(
                    m2m_field=field, join_to_query=join_to_query, alias=alias,
                    m2m_is_on_this_model=True)

        return self.add_join(query_set, info['rhs_table'], info['rhs_column'],
                             join_from_key=info['lhs_column'],
                             join_condition=info['where_clause'],
                             join_condition_values=info['values'],
                             alias=alias,
                             force_left_join=left_join)


    def key_on_joined_table(self, join_to_query):
        """Get a non-null column on the table joined for the given query.

        This analyzes the join that would be produced if join_to_query were
        passed to join_custom_field.
        """
        relationship_type, field = self.determine_relationship(
                join_to_query.model)
        if relationship_type == self.MANY_TO_ONE:
            return join_to_query.model._meta.pk.column
        return field.m2m_column_name() # any column on the M2M table will do


    def add_where(self, query_set, where, values=()):
        query_set = query_set.all()
        query_set.query.where.add(self._WhereClause(where, values),
                                  django.db.models.sql.where.AND)
        return query_set


    def _get_quoted_field(self, table, field):
        return _quote_name(table) + '.' + _quote_name(field)


    def get_key_on_this_table(self, key_field=None):
        if key_field is None:
            # default to primary key
            key_field = self.model._meta.pk.column
        return self._get_quoted_field(self.model._meta.db_table, key_field)


    def escape_user_sql(self, sql):
        return sql.replace('%', '%%')


    def _custom_select_query(self, query_set, selects):
        """Execute a custom select query.

        @param query_set: query set as returned by query_objects.
        @param selects: Tables/Columns to select, e.g. tko_test_labels_list.id.

        @returns: Result of the query as returned by cursor.fetchall().
        """
        compiler = query_set.query.get_compiler(using=query_set.db)
        sql, params = compiler.as_sql()
        from_ = sql[sql.find(' FROM'):]

        if query_set.query.distinct:
            distinct = 'DISTINCT '
        else:
            distinct = ''

        sql_query = ('SELECT ' + distinct + ','.join(selects) + from_)
        # Chose the connection that's responsible for this type of object
        cursor = connections[query_set.db].cursor()
        cursor.execute(sql_query, params)
        return cursor.fetchall()


    def _is_relation_to(self, field, model_class):
        return field.rel and field.rel.to is model_class


    MANY_TO_ONE = object()
    M2M_ON_RELATED_MODEL = object()
    M2M_ON_THIS_MODEL = object()

    def determine_relationship(self, related_model):
        """
        Determine the relationship between this model and related_model.

        related_model must have some sort of many-valued relationship to this
        manager's model.
        @returns (relationship_type, field), where relationship_type is one of
                MANY_TO_ONE, M2M_ON_RELATED_MODEL, M2M_ON_THIS_MODEL, and field
                is the Django field object for the relationship.
        """
        # look for a foreign key field on related_model relating to this model
        for field in related_model._meta.fields:
            if self._is_relation_to(field, self.model):
                return self.MANY_TO_ONE, field

        # look for an M2M field on related_model relating to this model
        for field in related_model._meta.many_to_many:
            if self._is_relation_to(field, self.model):
                return self.M2M_ON_RELATED_MODEL, field

        # maybe this model has the many-to-many field
        for field in self.model._meta.many_to_many:
            if self._is_relation_to(field, related_model):
                return self.M2M_ON_THIS_MODEL, field

        raise ValueError('%s has no relation to %s' %
                         (related_model, self.model))


    def _get_pivot_iterator(self, base_objects_by_id, related_model):
        """
        Determine the relationship between this model and related_model, and
        return a pivot iterator.
        @param base_objects_by_id: dict of instances of this model indexed by
        their IDs
        @returns a pivot iterator, which yields a tuple (base_object,
        related_object) for each relationship between a base object and a
        related object.  all base_object instances come from base_objects_by_id.
        Note -- this depends on Django model internals.
        """
        relationship_type, field = self.determine_relationship(related_model)
        if relationship_type == self.MANY_TO_ONE:
            return self._many_to_one_pivot(base_objects_by_id,
                                           related_model, field)
        elif relationship_type == self.M2M_ON_RELATED_MODEL:
            return self._many_to_many_pivot(
                    base_objects_by_id, related_model, field.m2m_db_table(),
                    field.m2m_reverse_name(), field.m2m_column_name())
        else:
            assert relationship_type == self.M2M_ON_THIS_MODEL
            return self._many_to_many_pivot(
                    base_objects_by_id, related_model, field.m2m_db_table(),
                    field.m2m_column_name(), field.m2m_reverse_name())


    def _many_to_one_pivot(self, base_objects_by_id, related_model,
                           foreign_key_field):
        """
        @returns a pivot iterator - see _get_pivot_iterator()
        """
        filter_data = {foreign_key_field.name + '__pk__in':
                       base_objects_by_id.keys()}
        for related_object in related_model.objects.filter(**filter_data):
            # lookup base object in the dict, rather than grabbing it from the
            # related object.  we need to return instances from the dict, not
            # fresh instances of the same models (and grabbing model instances
            # from the related models incurs a DB query each time).
            base_object_id = getattr(related_object, foreign_key_field.attname)
            base_object = base_objects_by_id[base_object_id]
            yield base_object, related_object


    def _query_pivot_table(self, base_objects_by_id, pivot_table,
                           pivot_from_field, pivot_to_field, related_model):
        """
        @param id_list list of IDs of self.model objects to include
        @param pivot_table the name of the pivot table
        @param pivot_from_field a field name on pivot_table referencing
        self.model
        @param pivot_to_field a field name on pivot_table referencing the
        related model.
        @param related_model the related model

        @returns pivot list of IDs (base_id, related_id)
        """
        query = """
        SELECT %(from_field)s, %(to_field)s
        FROM %(table)s
        WHERE %(from_field)s IN (%(id_list)s)
        """ % dict(from_field=pivot_from_field,
                   to_field=pivot_to_field,
                   table=pivot_table,
                   id_list=','.join(str(id_) for id_
                                    in base_objects_by_id.iterkeys()))

        # Chose the connection that's responsible for this type of object
        # The databases for related_model and the current model will always
        # be the same, related_model is just easier to obtain here because
        # self is only a ExtendedManager, not the object.
        cursor = connections[related_model.objects.db].cursor()
        cursor.execute(query)
        return cursor.fetchall()


    def _many_to_many_pivot(self, base_objects_by_id, related_model,
                            pivot_table, pivot_from_field, pivot_to_field):
        """
        @param pivot_table: see _query_pivot_table
        @param pivot_from_field: see _query_pivot_table
        @param pivot_to_field: see _query_pivot_table
        @returns a pivot iterator - see _get_pivot_iterator()
        """
        id_pivot = self._query_pivot_table(base_objects_by_id, pivot_table,
                                           pivot_from_field, pivot_to_field,
                                           related_model)

        all_related_ids = list(set(related_id for base_id, related_id
                                   in id_pivot))
        related_objects_by_id = related_model.objects.in_bulk(all_related_ids)

        for base_id, related_id in id_pivot:
            yield base_objects_by_id[base_id], related_objects_by_id[related_id]


    def populate_relationships(self, base_objects, related_model,
                               related_list_name):
        """
        For each instance of this model in base_objects, add a field named
        related_list_name listing all the related objects of type related_model.
        related_model must be in a many-to-one or many-to-many relationship with
        this model.
        @param base_objects - list of instances of this model
        @param related_model - model class related to this model
        @param related_list_name - attribute name in which to store the related
        object list.
        """
        if not base_objects:
            # if we don't bail early, we'll get a SQL error later
            return

        base_objects_by_id = dict((base_object._get_pk_val(), base_object)
                                  for base_object in base_objects)
        pivot_iterator = self._get_pivot_iterator(base_objects_by_id,
                                                  related_model)

        for base_object in base_objects:
            setattr(base_object, related_list_name, [])

        for base_object, related_object in pivot_iterator:
            getattr(base_object, related_list_name).append(related_object)


class ModelWithInvalidQuerySet(dbmodels.query.QuerySet):
    """
    QuerySet that handles delete() properly for models with an "invalid" bit
    """
    def delete(self):
        for model in self:
            model.delete()


class ModelWithInvalidManager(ExtendedManager):
    """
    Manager for objects with an "invalid" bit
    """
    def get_query_set(self):
        return ModelWithInvalidQuerySet(self.model)


class ValidObjectsManager(ModelWithInvalidManager):
    """
    Manager returning only objects with invalid=False.
    """
    def get_query_set(self):
        queryset = super(ValidObjectsManager, self).get_query_set()
        return queryset.filter(invalid=False)


class ModelExtensions(rdb_model_extensions.ModelValidators):
    """\
    Mixin with convenience functions for models, built on top of
    the model validators in rdb_model_extensions.
    """
    # TODO: at least some of these functions really belong in a custom
    # Manager class


    SERIALIZATION_LINKS_TO_FOLLOW = set()
    """
    To be able to send jobs and hosts to shards, it's necessary to find their
    dependencies.
    The most generic approach for this would be to traverse all relationships
    to other objects recursively. This would list all objects that are related
    in any way.
    But this approach finds too many objects: If a host should be transferred,
    all it's relationships would be traversed. This would find an acl group.
    If then the acl group's relationships are traversed, the relationship
    would be followed backwards and many other hosts would be found.

    This mapping tells that algorithm which relations to follow explicitly.
    """


    SERIALIZATION_LINKS_TO_KEEP = set()
    """This set stores foreign keys which we don't want to follow, but
    still want to include in the serialized dictionary. For
    example, we follow the relationship `Host.hostattribute_set`,
    but we do not want to follow `HostAttributes.host_id` back to
    to Host, which would otherwise lead to a circle. However, we still
    like to serialize HostAttribute.`host_id`."""

    SERIALIZATION_LOCAL_LINKS_TO_UPDATE = set()
    """
    On deserializion, if the object to persist already exists, local fields
    will only be updated, if their name is in this set.
    """


    @classmethod
    def convert_human_readable_values(cls, data, to_human_readable=False):
        """\
        Performs conversions on user-supplied field data, to make it
        easier for users to pass human-readable data.

        For all fields that have choice sets, convert their values
        from human-readable strings to enum values, if necessary.  This
        allows users to pass strings instead of the corresponding
        integer values.

        For all foreign key fields, call smart_get with the supplied
        data.  This allows the user to pass either an ID value or
        the name of the object as a string.

        If to_human_readable=True, perform the inverse - i.e. convert
        numeric values to human readable values.

        This method modifies data in-place.
        """
        field_dict = cls.get_field_dict()
        for field_name in data:
            if field_name not in field_dict or data[field_name] is None:
                continue
            field_obj = field_dict[field_name]
            # convert enum values
            if field_obj.choices:
                for choice_data in field_obj.choices:
                    # choice_data is (value, name)
                    if to_human_readable:
                        from_val, to_val = choice_data
                    else:
                        to_val, from_val = choice_data
                    if from_val == data[field_name]:
                        data[field_name] = to_val
                        break
            # convert foreign key values
            elif field_obj.rel:
                dest_obj = field_obj.rel.to.smart_get(data[field_name],
                                                      valid_only=False)
                if to_human_readable:
                    # parameterized_jobs do not have a name_field
                    if (field_name != 'parameterized_job' and
                        dest_obj.name_field is not None):
                        data[field_name] = getattr(dest_obj,
                                                   dest_obj.name_field)
                else:
                    data[field_name] = dest_obj




    def _validate_unique(self):
        """\
        Validate that unique fields are unique.  Django manipulators do
        this too, but they're a huge pain to use manually.  Trust me.
        """
        errors = {}
        cls = type(self)
        field_dict = self.get_field_dict()
        manager = cls.get_valid_manager()
        for field_name, field_obj in field_dict.iteritems():
            if not field_obj.unique:
                continue

            value = getattr(self, field_name)
            if value is None and field_obj.auto_created:
                # don't bother checking autoincrement fields about to be
                # generated
                continue

            existing_objs = manager.filter(**{field_name : value})
            num_existing = existing_objs.count()

            if num_existing == 0:
                continue
            if num_existing == 1 and existing_objs[0].id == self.id:
                continue
            errors[field_name] = (
                'This value must be unique (%s)' % (value))
        return errors


    def _validate(self):
        """
        First coerces all fields on this instance to their proper Python types.
        Then runs validation on every field. Returns a dictionary of
        field_name -> error_list.

        Based on validate() from django.db.models.Model in Django 0.96, which
        was removed in Django 1.0. It should reappear in a later version. See:
            http://code.djangoproject.com/ticket/6845
        """
        error_dict = {}
        for f in self._meta.fields:
            try:
                python_value = f.to_python(
                    getattr(self, f.attname, f.get_default()))
            except django.core.exceptions.ValidationError, e:
                error_dict[f.name] = str(e)
                continue

            if not f.blank and not python_value:
                error_dict[f.name] = 'This field is required.'
                continue

            setattr(self, f.attname, python_value)

        return error_dict


    def do_validate(self):
        errors = self._validate()
        unique_errors = self._validate_unique()
        for field_name, error in unique_errors.iteritems():
            errors.setdefault(field_name, error)
        if errors:
            raise ValidationError(errors)


    # actually (externally) useful methods follow

    @classmethod
    def add_object(cls, data={}, **kwargs):
        """\
        Returns a new object created with the given data (a dictionary
        mapping field names to values). Merges any extra keyword args
        into data.
        """
        data = dict(data)
        data.update(kwargs)
        data = cls.prepare_data_args(data)
        cls.convert_human_readable_values(data)
        data = cls.provide_default_values(data)

        obj = cls(**data)
        obj.do_validate()
        obj.save()
        return obj


    def update_object(self, data={}, **kwargs):
        """\
        Updates the object with the given data (a dictionary mapping
        field names to values).  Merges any extra keyword args into
        data.
        """
        data = dict(data)
        data.update(kwargs)
        data = self.prepare_data_args(data)
        self.convert_human_readable_values(data)
        for field_name, value in data.iteritems():
            setattr(self, field_name, value)
        self.do_validate()
        self.save()


    # see query_objects()
    _SPECIAL_FILTER_KEYS = ('query_start', 'query_limit', 'sort_by',
                            'extra_args', 'extra_where', 'no_distinct')


    @classmethod
    def _extract_special_params(cls, filter_data):
        """
        @returns a tuple of dicts (special_params, regular_filters), where
        special_params contains the parameters we handle specially and
        regular_filters is the remaining data to be handled by Django.
        """
        regular_filters = dict(filter_data)
        special_params = {}
        for key in cls._SPECIAL_FILTER_KEYS:
            if key in regular_filters:
                special_params[key] = regular_filters.pop(key)
        return special_params, regular_filters


    @classmethod
    def apply_presentation(cls, query, filter_data):
        """
        Apply presentation parameters -- sorting and paging -- to the given
        query.
        @returns new query with presentation applied
        """
        special_params, _ = cls._extract_special_params(filter_data)
        sort_by = special_params.get('sort_by', None)
        if sort_by:
            assert isinstance(sort_by, list) or isinstance(sort_by, tuple)
            query = query.extra(order_by=sort_by)

        query_start = special_params.get('query_start', None)
        query_limit = special_params.get('query_limit', None)
        if query_start is not None:
            if query_limit is None:
                raise ValueError('Cannot pass query_start without query_limit')
            # query_limit is passed as a page size
            query_limit += query_start
        return query[query_start:query_limit]


    @classmethod
    def query_objects(cls, filter_data, valid_only=True, initial_query=None,
                      apply_presentation=True):
        """\
        Returns a QuerySet object for querying the given model_class
        with the given filter_data.  Optional special arguments in
        filter_data include:
        -query_start: index of first return to return
        -query_limit: maximum number of results to return
        -sort_by: list of fields to sort on.  prefixing a '-' onto a
         field name changes the sort to descending order.
        -extra_args: keyword args to pass to query.extra() (see Django
         DB layer documentation)
        -extra_where: extra WHERE clause to append
        -no_distinct: if True, a DISTINCT will not be added to the SELECT
        """
        special_params, regular_filters = cls._extract_special_params(
                filter_data)

        if initial_query is None:
            if valid_only:
                initial_query = cls.get_valid_manager()
            else:
                initial_query = cls.objects

        query = initial_query.filter(**regular_filters)

        use_distinct = not special_params.get('no_distinct', False)
        if use_distinct:
            query = query.distinct()

        extra_args = special_params.get('extra_args', {})
        extra_where = special_params.get('extra_where', None)
        if extra_where:
            # escape %'s
            extra_where = cls.objects.escape_user_sql(extra_where)
            extra_args.setdefault('where', []).append(extra_where)
        if extra_args:
            query = query.extra(**extra_args)
            # TODO: Use readonly connection for these queries.
            # This has been disabled, because it's not used anyway, as the
            # configured readonly user is the same as the real user anyway.

        if apply_presentation:
            query = cls.apply_presentation(query, filter_data)

        return query


    @classmethod
    def query_count(cls, filter_data, initial_query=None):
        """\
        Like query_objects, but retreive only the count of results.
        """
        filter_data.pop('query_start', None)
        filter_data.pop('query_limit', None)
        query = cls.query_objects(filter_data, initial_query=initial_query)
        return query.count()


    @classmethod
    def clean_object_dicts(cls, field_dicts):
        """\
        Take a list of dicts corresponding to object (as returned by
        query.values()) and clean the data to be more suitable for
        returning to the user.
        """
        for field_dict in field_dicts:
            cls.clean_foreign_keys(field_dict)
            cls._convert_booleans(field_dict)
            cls.convert_human_readable_values(field_dict,
                                              to_human_readable=True)


    @classmethod
    def list_objects(cls, filter_data, initial_query=None):
        """\
        Like query_objects, but return a list of dictionaries.
        """
        query = cls.query_objects(filter_data, initial_query=initial_query)
        extra_fields = query.query.extra_select.keys()
        field_dicts = [model_object.get_object_dict(extra_fields=extra_fields)
                       for model_object in query]
        return field_dicts


    @classmethod
    def smart_get(cls, id_or_name, valid_only=True):
        """\
        smart_get(integer) -> get object by ID
        smart_get(string) -> get object by name_field
        """
        if valid_only:
            manager = cls.get_valid_manager()
        else:
            manager = cls.objects

        if isinstance(id_or_name, (int, long)):
            return manager.get(pk=id_or_name)
        if isinstance(id_or_name, basestring) and hasattr(cls, 'name_field'):
            return manager.get(**{cls.name_field : id_or_name})
        raise ValueError(
            'Invalid positional argument: %s (%s)' % (id_or_name,
                                                      type(id_or_name)))


    @classmethod
    def smart_get_bulk(cls, id_or_name_list):
        invalid_inputs = []
        result_objects = []
        for id_or_name in id_or_name_list:
            try:
                result_objects.append(cls.smart_get(id_or_name))
            except cls.DoesNotExist:
                invalid_inputs.append(id_or_name)
        if invalid_inputs:
            raise cls.DoesNotExist('The following %ss do not exist: %s'
                                   % (cls.__name__.lower(),
                                      ', '.join(invalid_inputs)))
        return result_objects


    def get_object_dict(self, extra_fields=None):
        """\
        Return a dictionary mapping fields to this object's values.  @param
        extra_fields: list of extra attribute names to include, in addition to
        the fields defined on this object.
        """
        fields = self.get_field_dict().keys()
        if extra_fields:
            fields += extra_fields
        object_dict = dict((field_name, getattr(self, field_name))
                           for field_name in fields)
        self.clean_object_dicts([object_dict])
        self._postprocess_object_dict(object_dict)
        return object_dict


    def _postprocess_object_dict(self, object_dict):
        """For subclasses to override."""
        pass


    @classmethod
    def get_valid_manager(cls):
        return cls.objects


    def _record_attributes(self, attributes):
        """
        See on_attribute_changed.
        """
        assert not isinstance(attributes, basestring)
        self._recorded_attributes = dict((attribute, getattr(self, attribute))
                                         for attribute in attributes)


    def _check_for_updated_attributes(self):
        """
        See on_attribute_changed.
        """
        for attribute, original_value in self._recorded_attributes.iteritems():
            new_value = getattr(self, attribute)
            if original_value != new_value:
                self.on_attribute_changed(attribute, original_value)
        self._record_attributes(self._recorded_attributes.keys())


    def on_attribute_changed(self, attribute, old_value):
        """
        Called whenever an attribute is updated.  To be overridden.

        To use this method, you must:
        * call _record_attributes() from __init__() (after making the super
        call) with a list of attributes for which you want to be notified upon
        change.
        * call _check_for_updated_attributes() from save().
        """
        pass


    def serialize(self, include_dependencies=True):
        """Serializes the object with dependencies.

        The variable SERIALIZATION_LINKS_TO_FOLLOW defines which dependencies
        this function will serialize with the object.

        @param include_dependencies: Whether or not to follow relations to
                                     objects this object depends on.
                                     This parameter is used when uploading
                                     jobs from a shard to the master, as the
                                     master already has all the dependent
                                     objects.

        @returns: Dictionary representation of the object.
        """
        serialized = {}
        timer = autotest_stats.Timer('serialize_latency.%s' % (
                type(self).__name__))
        with timer.get_client('local'):
            for field in self._meta.concrete_model._meta.local_fields:
                if field.rel is None:
                    serialized[field.name] = field._get_val_from_obj(self)
                elif field.name in self.SERIALIZATION_LINKS_TO_KEEP:
                    # attname will contain "_id" suffix for foreign keys,
                    # e.g. HostAttribute.host will be serialized as 'host_id'.
                    # Use it for easy deserialization.
                    serialized[field.attname] = field._get_val_from_obj(self)

        if include_dependencies:
            with timer.get_client('related'):
                for link in self.SERIALIZATION_LINKS_TO_FOLLOW:
                    serialized[link] = self._serialize_relation(link)

        return serialized


    def _serialize_relation(self, link):
        """Serializes dependent objects given the name of the relation.

        @param link: Name of the relation to take objects from.

        @returns For To-Many relationships a list of the serialized related
            objects, for To-One relationships the serialized related object.
        """
        try:
            attr = getattr(self, link)
        except AttributeError:
            # One-To-One relationships that point to None may raise this
            return None

        if attr is None:
            return None
        if hasattr(attr, 'all'):
            return [obj.serialize() for obj in attr.all()]
        return attr.serialize()


    @classmethod
    def _split_local_from_foreign_values(cls, data):
        """This splits local from foreign values in a serialized object.

        @param data: The serialized object.

        @returns A tuple of two lists, both containing tuples in the form
                 (link_name, link_value). The first list contains all links
                 for local fields, the second one contains those for foreign
                 fields/objects.
        """
        links_to_local_values, links_to_related_values = [], []
        for link, value in data.iteritems():
            if link in cls.SERIALIZATION_LINKS_TO_FOLLOW:
                # It's a foreign key
                links_to_related_values.append((link, value))
            else:
                # It's a local attribute or a foreign key
                # we don't want to follow.
                links_to_local_values.append((link, value))
        return links_to_local_values, links_to_related_values


    @classmethod
    def _filter_update_allowed_fields(cls, data):
        """Filters data and returns only files that updates are allowed on.

        This is i.e. needed for syncing aborted bits from the master to shards.

        Local links are only allowed to be updated, if they are in
        SERIALIZATION_LOCAL_LINKS_TO_UPDATE.
        Overwriting existing values is allowed in order to be able to sync i.e.
        the aborted bit from the master to a shard.

        The whitelisting mechanism is in place to prevent overwriting local
        status: If all fields were overwritten, jobs would be completely be
        set back to their original (unstarted) state.

        @param data: List with tuples of the form (link_name, link_value), as
                     returned by _split_local_from_foreign_values.

        @returns List of the same format as data, but only containing data for
                 fields that updates are allowed on.
        """
        return [pair for pair in data
                if pair[0] in cls.SERIALIZATION_LOCAL_LINKS_TO_UPDATE]


    @classmethod
    def delete_matching_record(cls, **filter_args):
        """Delete records matching the filter.

        @param filter_args: Arguments for the django filter
                used to locate the record to delete.
        """
        try:
            existing_record = cls.objects.get(**filter_args)
        except cls.DoesNotExist:
            return
        existing_record.delete()


    def _deserialize_local(self, data):
        """Set local attributes from a list of tuples.

        @param data: List of tuples like returned by
                     _split_local_from_foreign_values.
        """
        if not data:
            return

        for link, value in data:
            setattr(self, link, value)
        # Overwridden save() methods are prone to errors, so don't execute them.
        # This is because:
        # - the overwritten methods depend on ACL groups that don't yet exist
        #   and don't handle errors
        # - the overwritten methods think this object already exists in the db
        #   because the id is already set
        super(type(self), self).save()


    def _deserialize_relations(self, data):
        """Set foreign attributes from a list of tuples.

        This deserialized the related objects using their own deserialize()
        function and then sets the relation.

        @param data: List of tuples like returned by
                     _split_local_from_foreign_values.
        """
        for link, value in data:
            self._deserialize_relation(link, value)
        # See comment in _deserialize_local
        super(type(self), self).save()


    @classmethod
    def get_record(cls, data):
        """Retrieve a record with the data in the given input arg.

        @param data: A dictionary containing the information to use in a query
                for data. If child models have different constraints of
                uniqueness they should override this model.

        @return: An object with matching data.

        @raises DoesNotExist: If a record with the given data doesn't exist.
        """
        return cls.objects.get(id=data['id'])


    @classmethod
    def deserialize(cls, data):
        """Recursively deserializes and saves an object with it's dependencies.

        This takes the result of the serialize method and creates objects
        in the database that are just like the original.

        If an object of the same type with the same id already exists, it's
        local values will be left untouched, unless they are explicitly
        whitelisted in SERIALIZATION_LOCAL_LINKS_TO_UPDATE.

        Deserialize will always recursively propagate to all related objects
        present in data though.
        I.e. this is necessary to add users to an already existing acl-group.

        @param data: Representation of an object and its dependencies, as
                     returned by serialize.

        @returns: The object represented by data if it didn't exist before,
                  otherwise the object that existed before and has the same type
                  and id as the one described by data.
        """
        if data is None:
            return None

        local, related = cls._split_local_from_foreign_values(data)
        try:
            instance = cls.get_record(data)
            local = cls._filter_update_allowed_fields(local)
        except cls.DoesNotExist:
            instance = cls()

        timer = autotest_stats.Timer('deserialize_latency.%s' % (
                type(instance).__name__))
        with timer.get_client('local'):
            instance._deserialize_local(local)
        with timer.get_client('related'):
            instance._deserialize_relations(related)

        return instance


    def sanity_check_update_from_shard(self, shard, updated_serialized,
                                       *args, **kwargs):
        """Check if an update sent from a shard is legitimate.

        @raises error.UnallowedRecordsSentToMaster if an update is not
                legitimate.
        """
        raise NotImplementedError(
            'sanity_check_update_from_shard must be implemented by subclass %s '
            'for type %s' % type(self))


    @transaction.commit_on_success
    def update_from_serialized(self, serialized):
        """Updates local fields of an existing object from a serialized form.

        This is different than the normal deserialize() in the way that it
        does update local values, which deserialize doesn't, but doesn't
        recursively propagate to related objects, which deserialize() does.

        The use case of this function is to update job records on the master
        after the jobs have been executed on a slave, as the master is not
        interested in updates for users, labels, specialtasks, etc.

        @param serialized: Representation of an object and its dependencies, as
                           returned by serialize.

        @raises ValueError: if serialized contains related objects, i.e. not
                            only local fields.
        """
        local, related = (
            self._split_local_from_foreign_values(serialized))
        if related:
            raise ValueError('Serialized must not contain foreign '
                             'objects: %s' % related)

        self._deserialize_local(local)


    def custom_deserialize_relation(self, link, data):
        """Allows overriding the deserialization behaviour by subclasses."""
        raise NotImplementedError(
            'custom_deserialize_relation must be implemented by subclass %s '
            'for relation %s' % (type(self), link))


    def _deserialize_relation(self, link, data):
        """Deserializes related objects and sets references on this object.

        Relations that point to a list of objects are handled automatically.
        For many-to-one or one-to-one relations custom_deserialize_relation
        must be overridden by the subclass.

        Related objects are deserialized using their deserialize() method.
        Thereby they and their dependencies are created if they don't exist
        and saved to the database.

        @param link: Name of the relation.
        @param data: Serialized representation of the related object(s).
                     This means a list of dictionaries for to-many relations,
                     just a dictionary for to-one relations.
        """
        field = getattr(self, link)

        if field and hasattr(field, 'all'):
            self._deserialize_2m_relation(link, data, field.model)
        else:
            self.custom_deserialize_relation(link, data)


    def _deserialize_2m_relation(self, link, data, related_class):
        """Deserialize related objects for one to-many relationship.

        @param link: Name of the relation.
        @param data: Serialized representation of the related objects.
                     This is a list with of dictionaries.
        @param related_class: A class representing a django model, with which
                              this class has a one-to-many relationship.
        """
        relation_set = getattr(self, link)
        if related_class == self.get_attribute_model():
            # When deserializing a model together with
            # its attributes, clear all the exising attributes to ensure
            # db consistency. Note 'update' won't be sufficient, as we also
            # want to remove any attributes that no longer exist in |data|.
            #
            # core_filters is a dictionary of filters, defines how
            # RelatedMangager would query for the 1-to-many relationship. E.g.
            # Host.objects.get(
            #     id=20).hostattribute_set.core_filters = {host_id:20}
            # We use it to delete objects related to the current object.
            related_class.objects.filter(**relation_set.core_filters).delete()
        for serialized in data:
            relation_set.add(related_class.deserialize(serialized))


    @classmethod
    def get_attribute_model(cls):
        """Return the attribute model.

        Subclass with attribute-like model should override this to
        return the attribute model class. This method will be
        called by _deserialize_2m_relation to determine whether
        to clear the one-to-many relations first on deserialization of object.
        """
        return None


class ModelWithInvalid(ModelExtensions):
    """
    Overrides model methods save() and delete() to support invalidation in
    place of actual deletion.  Subclasses must have a boolean "invalid"
    field.
    """

    def save(self, *args, **kwargs):
        first_time = (self.id is None)
        if first_time:
            # see if this object was previously added and invalidated
            my_name = getattr(self, self.name_field)
            filters = {self.name_field : my_name, 'invalid' : True}
            try:
                old_object = self.__class__.objects.get(**filters)
                self.resurrect_object(old_object)
            except self.DoesNotExist:
                # no existing object
                pass

        super(ModelWithInvalid, self).save(*args, **kwargs)


    def resurrect_object(self, old_object):
        """
        Called when self is about to be saved for the first time and is actually
        "undeleting" a previously deleted object.  Can be overridden by
        subclasses to copy data as desired from the deleted entry (but this
        superclass implementation must normally be called).
        """
        self.id = old_object.id


    def clean_object(self):
        """
        This method is called when an object is marked invalid.
        Subclasses should override this to clean up relationships that
        should no longer exist if the object were deleted.
        """
        pass


    def delete(self):
        self.invalid = self.invalid
        assert not self.invalid
        self.invalid = True
        self.save()
        self.clean_object()


    @classmethod
    def get_valid_manager(cls):
        return cls.valid_objects


    class Manipulator(object):
        """
        Force default manipulators to look only at valid objects -
        otherwise they will match against invalid objects when checking
        uniqueness.
        """
        @classmethod
        def _prepare(cls, model):
            super(ModelWithInvalid.Manipulator, cls)._prepare(model)
            cls.manager = model.valid_objects


class ModelWithAttributes(object):
    """
    Mixin class for models that have an attribute model associated with them.
    The attribute model is assumed to have its value field named "value".
    """

    def _get_attribute_model_and_args(self, attribute):
        """
        Subclasses should override this to return a tuple (attribute_model,
        keyword_args), where attribute_model is a model class and keyword_args
        is a dict of args to pass to attribute_model.objects.get() to get an
        instance of the given attribute on this object.
        """
        raise NotImplementedError


    def set_attribute(self, attribute, value):
        attribute_model, get_args = self._get_attribute_model_and_args(
            attribute)
        attribute_object, _ = attribute_model.objects.get_or_create(**get_args)
        attribute_object.value = value
        attribute_object.save()


    def delete_attribute(self, attribute):
        attribute_model, get_args = self._get_attribute_model_and_args(
            attribute)
        try:
            attribute_model.objects.get(**get_args).delete()
        except attribute_model.DoesNotExist:
            pass


    def set_or_delete_attribute(self, attribute, value):
        if value is None:
            self.delete_attribute(attribute)
        else:
            self.set_attribute(attribute, value)


class ModelWithHashManager(dbmodels.Manager):
    """Manager for use with the ModelWithHash abstract model class"""

    def create(self, **kwargs):
        raise Exception('ModelWithHash manager should use get_or_create() '
                        'instead of create()')


    def get_or_create(self, **kwargs):
        kwargs['the_hash'] = self.model._compute_hash(**kwargs)
        return super(ModelWithHashManager, self).get_or_create(**kwargs)


class ModelWithHash(dbmodels.Model):
    """Superclass with methods for dealing with a hash column"""

    the_hash = dbmodels.CharField(max_length=40, unique=True)

    objects = ModelWithHashManager()

    class Meta:
        abstract = True


    @classmethod
    def _compute_hash(cls, **kwargs):
        raise NotImplementedError('Subclasses must override _compute_hash()')


    def save(self, force_insert=False, **kwargs):
        """Prevents saving the model in most cases

        We want these models to be immutable, so the generic save() operation
        will not work. These models should be instantiated through their the
        model.objects.get_or_create() method instead.

        The exception is that save(force_insert=True) will be allowed, since
        that creates a new row. However, the preferred way to make instances of
        these models is through the get_or_create() method.
        """
        if not force_insert:
            # Allow a forced insert to happen; if it's a duplicate, the unique
            # constraint will catch it later anyways
            raise Exception('ModelWithHash is immutable')
        super(ModelWithHash, self).save(force_insert=force_insert, **kwargs)
