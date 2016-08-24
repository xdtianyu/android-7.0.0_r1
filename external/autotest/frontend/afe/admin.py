"""Django 1.0 admin interface declarations."""

from django import forms
from django.contrib import admin, messages
from django.db import models as dbmodels
from django.forms.util import flatatt
from django.utils.encoding import smart_str
from django.utils.safestring import mark_safe

from autotest_lib.cli import rpc, site_host
from autotest_lib.frontend import settings
from autotest_lib.frontend.afe import model_logic, models


class SiteAdmin(admin.ModelAdmin):
    def formfield_for_dbfield(self, db_field, **kwargs):
        field = super(SiteAdmin, self).formfield_for_dbfield(db_field, **kwargs)
        if (db_field.rel and
                issubclass(db_field.rel.to, model_logic.ModelWithInvalid)):
            model = db_field.rel.to
            field.choices = model.valid_objects.all().values_list(
                    'id', model.name_field)
        return field


class ModelWithInvalidForm(forms.ModelForm):
    def validate_unique(self):
        # Don't validate name uniqueness if the duplicate model is invalid
        model = self.Meta.model
        filter_data = {
                model.name_field : self.cleaned_data[model.name_field],
                'invalid' : True
                }
        needs_remove = bool(self.Meta.model.objects.filter(**filter_data))
        if needs_remove:
            name_field = self.fields.pop(model.name_field)
        super(ModelWithInvalidForm, self).validate_unique()
        if needs_remove:
            self.fields[model.name_field] = name_field


class AtomicGroupForm(ModelWithInvalidForm):
    class Meta:
        model = models.AtomicGroup


class AtomicGroupAdmin(SiteAdmin):
    list_display = ('name', 'description', 'max_number_of_machines')

    form = AtomicGroupForm

    def queryset(self, request):
        return models.AtomicGroup.valid_objects

admin.site.register(models.AtomicGroup, AtomicGroupAdmin)


class LabelForm(ModelWithInvalidForm):
    class Meta:
        model = models.Label


class LabelAdmin(SiteAdmin):
    list_display = ('name', 'atomic_group', 'kernel_config')
    # Avoid a bug with the admin interface showing a select box pointed at an
    # AtomicGroup when this field is intentionally NULL such that editing a
    # label via the admin UI unintentionally sets an atomicgroup.
    raw_id_fields = ('atomic_group',)

    form = LabelForm

    def queryset(self, request):
        return models.Label.valid_objects

admin.site.register(models.Label, LabelAdmin)


class UserAdmin(SiteAdmin):
    list_display = ('login', 'access_level')
    search_fields = ('login',)

admin.site.register(models.User, UserAdmin)


class LabelsCommaSpacedWidget(forms.Widget):
    """A widget that renders the labels in a comman separated text field."""

    def render(self, name, value, attrs=None):
        """Convert label ids to names and render them in HTML.

        @param name: Name attribute of the HTML tag.
        @param value: A list of label ids to be rendered.
        @param attrs: A dict of extra attributes rendered in the HTML tag.
        @return: A Unicode string in HTML format.
        """
        final_attrs = self.build_attrs(attrs, type='text', name=name)

        if value:
            label_names =(models.Label.objects.filter(id__in=value)
                          .values_list('name', flat=True))
            value = ', '.join(label_names)
        else:
            value = ''
        final_attrs['value'] = smart_str(value)
        return mark_safe(u'<input%s />' % flatatt(final_attrs))

    def value_from_datadict(self, data, files, name):
        """Convert input string to a list of label ids.

        @param data: A dict of input data from HTML form. The keys are name
            attrs of HTML tags.
        @param files: A dict of input file names from HTML form. The keys are
            name attrs of HTML tags.
        @param name: The name attr of the HTML tag of labels.
        @return: A list of label ids in string. Return None if no label is
            specified.
        """
        label_names = data.get(name)
        if label_names:
            label_names = label_names.split(',')
            label_names = filter(None,
                                 [name.strip(', ') for name in label_names])
            label_ids = (models.Label.objects.filter(name__in=label_names)
                         .values_list('id', flat=True))
            return [str(label_id) for label_id in label_ids]


class HostForm(ModelWithInvalidForm):
    # A checkbox triggers label autodetection.
    labels_autodetection = forms.BooleanField(initial=True, required=False)

    def __init__(self, *args, **kwargs):
        super(HostForm, self).__init__(*args, **kwargs)
        self.fields['labels'].widget = LabelsCommaSpacedWidget()
        self.fields['labels'].help_text = ('Please enter a comma seperated '
                                           'list of labels.')

    def clean(self):
        """ ModelForm validation

        Ensure that a lock_reason is provided when locking a device.
        """
        cleaned_data = super(HostForm, self).clean()
        locked = cleaned_data.get('locked')
        lock_reason = cleaned_data.get('lock_reason')
        if locked and not lock_reason:
            raise forms.ValidationError(
                    'Please provide a lock reason when locking a device.')
        return cleaned_data

    class Meta:
        model = models.Host


class HostAttributeInline(admin.TabularInline):
    model = models.HostAttribute
    extra = 1


class HostAdmin(SiteAdmin):
    # TODO(showard) - showing platform requires a SQL query for
    # each row (since labels are many-to-many) - should we remove
    # it?
    list_display = ('hostname', 'platform', 'locked', 'status')
    list_filter = ('locked', 'protection', 'status')
    search_fields = ('hostname',)

    form = HostForm

    def __init__(self, model, admin_site):
        self.successful_hosts = []
        super(HostAdmin, self).__init__(model, admin_site)

    def add_view(self, request, form_url='', extra_context=None):
        """ Field layout for admin page.

        fields specifies the visibility and order of HostAdmin attributes
        displayed on the device addition page.

        @param request:  django request
        @param form_url: url
        @param extra_context: A dict used to alter the page view
        """
        self.fields = ('hostname', 'locked', 'lock_reason', 'leased',
                       'protection', 'labels', 'shard', 'labels_autodetection')
        return super(HostAdmin, self).add_view(request, form_url, extra_context)

    def change_view(self, request, obj_id, form_url='', extra_context=None):
        # Hide labels_autodetection when editing a host.
        self.fields = ('hostname', 'locked', 'lock_reason',
                       'leased', 'protection', 'labels')
        # Only allow editing host attributes when a host has been created.
        self.inlines = [
            HostAttributeInline,
        ]
        return super(HostAdmin, self).change_view(request,
                                                  obj_id,
                                                  form_url,
                                                  extra_context)

    def queryset(self, request):
        return models.Host.valid_objects

    def response_add(self, request, obj, post_url_continue=None):
        # Disable the 'save and continue editing option' when adding a host.
        if "_continue" in request.POST:
            request.POST = request.POST.copy()
            del request.POST['_continue']
        return super(HostAdmin, self).response_add(request,
                                                   obj,
                                                   post_url_continue)

    def save_model(self, request, obj, form, change):
        if not form.cleaned_data.get('labels_autodetection'):
            return super(HostAdmin, self).save_model(request, obj,
                                                     form, change)

        # Get submitted info from form.
        web_server = rpc.get_autotest_server()
        hostname = form.cleaned_data['hostname']
        hosts = [str(hostname)]
        platform = None
        locked = form.cleaned_data['locked']
        lock_reason = form.cleaned_data['lock_reason']
        labels = [label.name for label in form.cleaned_data['labels']]
        protection = form.cleaned_data['protection']
        acls = []

        # Pipe to cli to perform autodetection and create host.
        host_create_obj = site_host.site_host_create.construct_without_parse(
                web_server, hosts, platform,
                locked, lock_reason, labels, acls,
                protection)
        try:
            self.successful_hosts = host_create_obj.execute()
        except SystemExit:
            # Invalid server name.
            messages.error(request, 'Invalid server name %s.' % web_server)

        # Successful_hosts is an empty list if there's time out,
        # server error, or JSON error.
        if not self.successful_hosts:
            messages.error(request,
                           'Label autodetection failed. '
                           'Host created with selected labels.')
            super(HostAdmin, self).save_model(request, obj, form, change)

    def save_related(self, request, form, formsets, change):
        """Save many-to-many relations between host and labels."""
        # Skip save_related if autodetection succeeded, since cli has already
        # handled many-to-many relations.
        if not self.successful_hosts:
            super(HostAdmin, self).save_related(request,
                                                form,
                                                formsets,
                                                change)

admin.site.register(models.Host, HostAdmin)


class TestAdmin(SiteAdmin):
    fields = ('name', 'author', 'test_category', 'test_class',
              'test_time', 'sync_count', 'test_type', 'path',
              'dependencies', 'experimental', 'run_verify',
              'description')
    list_display = ('name', 'test_type', 'admin_description', 'sync_count')
    search_fields = ('name',)
    filter_horizontal = ('dependency_labels',)

admin.site.register(models.Test, TestAdmin)


class ProfilerAdmin(SiteAdmin):
    list_display = ('name', 'description')
    search_fields = ('name',)

admin.site.register(models.Profiler, ProfilerAdmin)


class AclGroupAdmin(SiteAdmin):
    list_display = ('name', 'description')
    search_fields = ('name',)
    filter_horizontal = ('users', 'hosts')

    def queryset(self, request):
        return models.AclGroup.objects.exclude(name='Everyone')

    def save_model(self, request, obj, form, change):
        super(AclGroupAdmin, self).save_model(request, obj, form, change)
        _orig_save_m2m = form.save_m2m

        def save_m2m():
            _orig_save_m2m()
            obj.perform_after_save(change)

        form.save_m2m = save_m2m

admin.site.register(models.AclGroup, AclGroupAdmin)


class DroneSetForm(forms.ModelForm):
    def __init__(self, *args, **kwargs):
        super(DroneSetForm, self).__init__(*args, **kwargs)
        drone_ids_used = set()
        for drone_set in models.DroneSet.objects.exclude(id=self.instance.id):
            drone_ids_used.update(drone_set.drones.values_list('id', flat=True))
        available_drones = models.Drone.objects.exclude(id__in=drone_ids_used)

        self.fields['drones'].widget.choices = [(drone.id, drone.hostname)
                                                for drone in available_drones]


class DroneSetAdmin(SiteAdmin):
    filter_horizontal = ('drones',)
    form = DroneSetForm

admin.site.register(models.DroneSet, DroneSetAdmin)

admin.site.register(models.Drone)


if settings.FULL_ADMIN:
    class JobAdmin(SiteAdmin):
        list_display = ('id', 'owner', 'name', 'control_type')
        filter_horizontal = ('dependency_labels',)

    admin.site.register(models.Job, JobAdmin)


    class IneligibleHostQueueAdmin(SiteAdmin):
        list_display = ('id', 'job', 'host')

    admin.site.register(models.IneligibleHostQueue, IneligibleHostQueueAdmin)


    class HostQueueEntryAdmin(SiteAdmin):
        list_display = ('id', 'job', 'host', 'status',
                        'meta_host')

    admin.site.register(models.HostQueueEntry, HostQueueEntryAdmin)

    admin.site.register(models.AbortedHostQueueEntry)
