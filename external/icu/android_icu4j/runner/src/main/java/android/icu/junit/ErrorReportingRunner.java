package android.icu.junit;

import java.lang.reflect.InvocationTargetException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.manipulation.Filter;
import org.junit.runner.manipulation.Filterable;
import org.junit.runner.manipulation.NoTestsRemainException;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;
import org.junit.runners.model.InitializationError;

/**
 * A copy of the JUnit 4.10 {@link org.junit.internal.runners.ErrorReportingRunner} class.
 *
 * <p>Modified as follows:</p>
 *
 * <ul>
 * <li>allows the class in error to be specified by name rather than {@link Class} object so that it
 * can be used for when the class could not be found. See
 * {@link #ErrorReportingRunner(String, Throwable)} and
 * {@link #fTestClassName}.
 * <li>supports filtering of the individual causes. See {@link #filter(Filter)}.
 * <li>uses the cause to construct the description allowing filtering on specific error messages.
 * See {@link #describeCause(Throwable)}.
 * </ul>
 *
 * See https://github.com/junit-team/junit/issues/1253
 */
// android-changed - implements Filterable
class ErrorReportingRunner extends Runner implements Filterable {
    private final List<Throwable> fCauses;

    // android-changed - changed type from Class<?> and renamed from fTestClass.
    private final String fTestClassName;

    public ErrorReportingRunner(String testClassName, Throwable cause) {
        fTestClassName = testClassName;
        // Take a copy so that they can be modified during filtering if necessary.
        fCauses = new ArrayList<>(getCauses(cause));
    }
    // end android-changed

    @Override
    public Description getDescription() {
        // android-changed - renamed from fTestClass.
        Description description= Description.createSuiteDescription(fTestClassName);
        // end android-changed
        for (Throwable each : fCauses)
            description.addChild(describeCause(each));
        return description;
    }

    @Override
    public void run(RunNotifier notifier) {
        for (Throwable each : fCauses)
            runCause(each, notifier);
    }

    // android-changed - added filtering support
    @Override
    public void filter(Filter filter) throws NoTestsRemainException {
        Iterator<Throwable> iterator = fCauses.iterator();
        while (iterator.hasNext()) {
            Throwable cause = iterator.next();
            Description description = describeCause(cause);
            if (!filter.shouldRun(description)) {
                iterator.remove();
            }
        }

        // If there are no causes left then throw an exception to cause the parent runner, if any,
        // to remove this runner from its list.
        if (fCauses.isEmpty()) {
            throw new NoTestsRemainException();
        }
    }
    // end android-changed

    @SuppressWarnings("deprecation")
    private List<Throwable> getCauses(Throwable cause) {
        if (cause instanceof InvocationTargetException)
            return getCauses(cause.getCause());
        if (cause instanceof InitializationError)
            return ((InitializationError) cause).getCauses();
        if (cause instanceof org.junit.internal.runners.InitializationError)
            return ((org.junit.internal.runners.InitializationError) cause)
                    .getCauses();
        return Arrays.asList(cause);
    }

    private Description describeCause(Throwable child) {
        // android-changed - create a description that incorporates the cause
        // Extract the first line of the message, exclude any special characters that could
        // cause problems with later parsing of the description.
        String message = child.getMessage();
        Matcher matcher = Pattern.compile("^([^()\n]*).*").matcher(message);
        if (matcher.matches()) {
            message = matcher.group(1);
        }

        // Create a suite description (need to use that method because the createTestDescription
        // methods use a class rather than a class name).
        String causeClassName = child.getClass().getName();
        return Description.createSuiteDescription(
                String.format("initializationError[%s: %s](%s)",
                        causeClassName, message, fTestClassName));
        // end android-changed
    }

    private void runCause(Throwable child, RunNotifier notifier) {
        Description description= describeCause(child);
        notifier.fireTestStarted(description);
        notifier.fireTestFailure(new Failure(description, child));
        notifier.fireTestFinished(description);
    }
}