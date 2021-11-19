import com.nvmUnsafe.*;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Constructor;
import java.nio.ByteBuffer;

import sun.misc.Cleaner;                                                                                           
import sun.misc.Unsafe;

public class Example {
	static com.nvmUnsafe.NVMUnsafe nvmPool;

	public static ByteBuffer allocateDirectBuffer(int size) {
		try {
			Class<?> cls = Class.forName("java.nio.DirectByteBuffer");
			Constructor<?> constructor = cls.getDeclaredConstructor(Long.TYPE, Integer.TYPE);
			constructor.setAccessible(true);

			Field cleanerField = cls.getDeclaredField("cleaner");
			cleanerField.setAccessible(true);

			long memory = nvmPool.nvmAllocateMemory(size);

			ByteBuffer buffer = (ByteBuffer) constructor.newInstance(memory, size);

			Cleaner cleaner = Cleaner.create(buffer, () -> nvmPool.nvmFreeMemory(memory));
			cleanerField.set(buffer, cleaner);

			return buffer;
		} catch (Exception e) {
			System.out.println("Error");
		}

		throw new IllegalStateException("unreachable");
	}
    

    public static void main (String[] args) {

        try {
            Field nvmUnsafeField = NVMUnsafe.class.getDeclaredField("theNVMUnsafe");
            nvmUnsafeField.setAccessible(true);
            nvmPool = (com.nvmUnsafe.NVMUnsafe) nvmUnsafeField.get(null);
        } catch (Throwable cause) {
            nvmPool = null;
        }

        nvmPool.nvmInitialPool("/mnt/pmem/file.txt", 34359738368L);

		ByteBuffer b1 = allocateDirectBuffer(10);

		byte a = 'a';
		for (int i = 0; i < 10; i++) {

			b1.put(a);
		}
		
		ByteBuffer b2 = allocateDirectBuffer(1073741824);

		for (int i = 0; i < 1073741824; i++) {

			b2.put(a);
		}
		
		ByteBuffer b3 = allocateDirectBuffer(1073741824);
		
		for (int i = 0; i < 1073741824; i++) {

			b3.put(a);
		}
		
		ByteBuffer b4 = allocateDirectBuffer(1073741824);
		
		for (int i = 0; i < 1073741824; i++) {

			b4.put(a);
		}
		
		ByteBuffer b5 = allocateDirectBuffer(1073741824);
		
		for (int i = 0; i < 1073741824; i++) {

			b5.put(a);
		}
		
		ByteBuffer b6 = allocateDirectBuffer(1073741824);
		
		for (int i = 0; i < 1073741824; i++) {

			b6.put(a);
		}
		
		ByteBuffer b7 = allocateDirectBuffer(1073741824);
		
		for (int i = 0; i < 1073741824; i++) {

			b7.put(a);
		}

        nvmPool.nvmDelete();
        System.out.println("Delete Memory");
    }
}
