import java.io.*;

public class extest extends Object
{
    public static void main (String args[])
    {
        try
        {
            RandomAccessFile file = new RandomAccessFile("test.file", "rw");
            
            file.seek(file.length());
        }
        catch (Exception exc)
        {
            System.out.println("could not open file");
        }
    }
}
