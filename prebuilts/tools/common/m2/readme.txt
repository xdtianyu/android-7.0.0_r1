repository/ is a maven repository with libraries used to perform
certain tasks without accessing an external repository. e.g:
 - Compile tools/base, tools/swt and tools/buildSrc using Gradle
 - Convert artifacts from a maven repository to a p2 repository
   using the p2-maven plugin

Certain dependencies are using only during the build process,
but others are runtime dependencies that get shipped with the 
SDK Tools. Such runtime dependencies must include a NOTICE file
next to the artifact or the build will fail.

There are a few different ways to add artifacts to these repositories:

1. Add a dependency to an existing project in tools/base, say ddmlib,
   and then run the cloneArtifacts task from the tools folder. 

2. Invoke the maven-install-plugin from the command line. For example,
   the following command was used to install the protobuf jar into
   the repository:
    $ mvn org.apache.maven.plugins:maven-install-plugin:2.5.1:install-file \
        -Dfile=$OUT/host-libprotobuf-java-2.3.0-lite.jar \
        -DgroupId=com.android.tools.external \
        -DartifactId=libprotobuf-java-lite \
        -Dversion=2.3.0 \
        -Dpackaging=jar \
        -DgeneratePom=true \
        -DlocalRepositoryPath=repo \
        -DcreateChecksum=true

3. Adding all the dependencies for a maven plugin can be accomplished
   as follows:
     - Create a maven settings.xml file with a pointer to an empty
       folder as the localRepository.
     - Run the maven task using that settings.xml
     - When the task runs, all the necessary artifacts will be downloaded
       into that local repository.
     - Copy over the contents of that repository into this folder.
