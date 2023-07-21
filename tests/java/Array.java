import java.io.*; 
import java.lang.*; 
import java.util.Scanner;
import java.lang.management.ManagementFactory;
import java.lang.management.MemoryPoolMXBean;
import java.util.concurrent.ConcurrentHashMap;
import java.util.ArrayList;

import java.lang.reflect.Field;

public class Array {
	private static final sun.misc.Unsafe _UNSAFE;

	static {
		try {
			Field unsafeField = sun.misc.Unsafe.class.getDeclaredField("theUnsafe");
			unsafeField.setAccessible(true);
			_UNSAFE = (sun.misc.Unsafe) unsafeField.get(null);
		} catch (Exception e) {
			throw new RuntimeException("SimplePartition: Failed to " + "get unsafe", e);
		}
	}

	public static void mem_info(String str) {
		System.out.println("=========================================");
		System.out.println(str + "\n");
		System.out.println("=========================================");
		for(MemoryPoolMXBean memoryPoolMXBean: ManagementFactory.getMemoryPoolMXBeans()){
			System.out.println(memoryPoolMXBean.getName());
			System.out.println(memoryPoolMXBean.getUsage().getUsed());
		}
	}

	public static void gc()
	{
		System.out.println("=========================================");
		System.out.println("Call GC");
		System.gc();
		System.out.println("=========================================");
	}

	public static void main(String args[]) throws Exception {
    //int num_elements = 2000000;
	  int num_elements = 1;
	  long sum;

    int[] array2 = new int[1];

    int[] array1 = new int[num_elements];
	  _UNSAFE.h2TagAndMoveRoot(array1, 0, 0);

	  //int[] array2 = new int[num_elements];
	  _UNSAFE.h2TagAndMoveRoot(array2, 1, 0);

	  for (int i = 0; i < num_elements; i++)
	      array2[i] = 333;
	  

	  gc();

	  for (int i = 0; i < num_elements; i++)
	      array1[i] = i*2;
	  
	  gc();
	  for (int i = 0; i < num_elements; i++)
	      array1[i] = i;
	  
	  gc();
	  
	  sum = 0;
	  for (int i = 0; i < num_elements; i++)
	      sum += array1[i];
	  
	  for (int i = 0; i < num_elements; i++)
	      array2[i] = i*4;
	  
	  gc();
	  
	  for (int i = 0; i < num_elements; i++)
	      array2[i] = 333;
	  
	  gc();
	  
	  sum = 0;
	  for (int i = 0; i < num_elements; i++)
	      sum += array1[i];
  }
}
