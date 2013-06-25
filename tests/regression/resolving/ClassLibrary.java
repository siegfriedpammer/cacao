import java.util.*;

public enum ClassLibrary {
	GNU_CLASSPATH,
	OPEN_JDK,
	UNKNOWN;

	static ClassLibrary getCurrent() {
		String vendor       = System.getProperty("java.vendor").intern();
		String runtime_name = System.getProperty("java.runtime.name").intern();
		String spec_version = System.getProperty("java.specification.version").intern();

		if (vendor == "GNU Classpath")
			return GNU_CLASSPATH;

		if (runtime_name == "IcedTea6 Runtime Environment")
			return OPEN_JDK;

		if (runtime_name == "IcedTea7 Runtime Environment")
			return OPEN_JDK;

		if (runtime_name == "OpenJDK Runtime Environment")
			return OPEN_JDK;

        System.out.println("WARNING: Unknown class library (" + vendor       + 
		                                                  "/" + runtime_name + 
		                                                  "/" + spec_version + 
		                                                  ")");
        return UNKNOWN;
    }

	public static void main(String[] args) {
		System.out.println("Current class library: " + getCurrent());

		Enumeration<?> props = System.getProperties().propertyNames();

		while (props.hasMoreElements()) {
			String prop = String.valueOf(props.nextElement());
			String str  = prop;

			str = "'" + str + "'";
			while (str.length() < 40) str = str + " ";


			System.out.printf("%s : '%s'\n", str, System.getProperty(prop));
		}
	}
}
