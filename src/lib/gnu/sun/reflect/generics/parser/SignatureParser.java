package sun.reflect.generics.parser;

import java.lang.reflect.Array;

public class SignatureParser {
	private int index = 0;
	private String signature;
	private ClassLoader classLoader;
	private Class<?> type = null;
	
	public SignatureParser(String signature, ClassLoader classLoader)
			throws SignatureFormatError, ClassNotFoundException {
		this.signature   = signature;
		this.classLoader = classLoader;
		
		try {
			type = parseSignature();
		}
		catch(IndexOutOfBoundsException e) {
			throw new SignatureFormatError(signature, e);
		}
		
		if (index != signature.length())
			throw new SignatureFormatError(signature);
	}
	
	public SignatureParser(String signature)
			throws SignatureFormatError, ClassNotFoundException {
		this(signature, ClassLoader.getSystemClassLoader());
	}
	
	public Class<?> getType() {
		return type;
	}
	
	private Class<?> parseSignature()
			throws SignatureFormatError, ClassNotFoundException {
		Class<?> type = null;
		char ch = signature.charAt(index);

		switch (ch) {
		case 'L':
			int endIndex = signature.indexOf(';', ++index);
			
			if (endIndex == -1) {
				throw new SignatureFormatError(signature);
			}
			
			type = Class.forName(signature.substring(index, endIndex
					).replace('/', '.'), true, classLoader);
			index = endIndex + 1;
			break;
		case '[':
			/* ignore optional array size */
			do {
				ch = signature.charAt(++ index);
			} while (ch >= '0' && ch <= '9');
			type = Array.newInstance(parseSignature(), 0).getClass();
			break;
		default:
			type = getPrimitiveClassForSignature(ch);
			++ index;
		}
		
		return type;
	}

	private Class<?> getPrimitiveClassForSignature(char typeCode)
			throws SignatureFormatError {
		switch (typeCode) {
		case 'B': return byte.class;
		case 'C': return char.class;
		case 'D': return double.class;
		case 'F': return float.class;
		case 'I': return int.class;
		case 'J': return long.class;
		case 'S': return short.class;
		case 'Z': return boolean.class;
		case 'V': return void.class;
		}
		
		throw new SignatureFormatError(signature);
	}
}
