# Releasing to JCenter

Creating a release on jcenter is done by invoking the binaryUpload task with a bintray user and API key. Your API key can be [found here](https://bintray.com/user/edit/tab/apikey).

`gradle -PbintrayUser=myusername -PbintrayKey=123 clean bintrayUpload`

Note that you'll need to be a member of the appium organization on jcenter before publishing. Existing members are able to invite new ones.

Update the version number in `build.gradle` by modifying the value of `ddVersion`. Official releases should be made only after removing the `-SNAPSHOT` suffix. If the same version number is used as an existing release of droiddriver then jcenter will reject the upload.

# Releasing snapshots to artifactory

Snapshots of DroidDriver are released to `http://oss.jfrog.org/artifactory` in the oss-snapshot-local
repository.

`gradle -PbintrayUser=myusername -PbintrayKey=123 clean assemble artifactoryPublish`

Note that resolving the snapshots requires adding the maven repo to the gradle build file:

`maven { url 'http://oss.jfrog.org/artifactory/oss-snapshot-local' }`

# Known Issues

- `[buildinfo] Properties file path was not found! (Relevant only for builds running on a CI Server)`
The missing properties warning can be safely ignored. We're populating the values in Gradle so
the artifactory plugin doesn't know that they're already set.
