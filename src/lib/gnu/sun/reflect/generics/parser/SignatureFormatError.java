package sun.reflect.generics.parser;

public class SignatureFormatError extends Exception {
	private static final long serialVersionUID = -1151912353615858866L;

	public SignatureFormatError() {
		super("illegal type signature");
	}

	public SignatureFormatError(String signature) {
		super("illegal type signature: \"" + signature + "\"");
	}

	public SignatureFormatError(Throwable cause) {
		super("illegal type signature", cause);
	}

	public SignatureFormatError(String signature, Throwable cause) {
		super("illegal type signature: \"" + signature + "\"", cause);
	}
}
