public class exception {

	public exception() {
	}

	public static void b(int v) throws Exception{
		throw new Exception("Exception: value="+v);
	}

	public static void a() throws Exception {
		b(1);
	}

	public static void main(String args[]){
		try {
			a();
		}
		catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
		}
		try {
			throw new Exception("2");
		}
		catch (Exception e) {
			System.out.println(e.getMessage());
			e.printStackTrace();
			
		}

		throw new ClassCastException();
	}
}
