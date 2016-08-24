// RUN: %Slang %s
// RUN: %rs-filecheck-wrapper %s
// CHECK: call void @_Z13rsClearObjectP10rs_element(%struct.rs_element{{.*}}* nonnull %.rs.tmp{{[0-9]+}})
// CHECK: call void @_Z11rsSetObjectP10rs_elementS_(%struct.rs_element{{.*}}* nonnull %.rs.retval{{[0-9]+}}, {{.*}})

#pragma version(1)
#pragma rs java_package_name(ref_count)

static rs_element bar() {
  rs_element x = {0};
  return x;
}

void entrypoint() {
  rs_element e = bar();
  if (rsIsObject(e)) {
    rsDebug("good object", 0);
  }
}


