/** tests/compiler2/junit/Fact.java - Fact
 *
 * Copyright (C) 1996-2014
 * CACAOVM - Verein zur Foerderung der freien virtuellen Maschine CACAO
 *
 * This file is part of CACAO.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
package org.cacaojvm.compiler2.test;

import java.util.Arrays;
import java.util.Collection;
import java.util.Random;

import org.junit.Test;
import org.junit.Ignore;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;

import org.junit.FixMethodOrder;
import org.junit.runners.MethodSorters;
import static org.junit.Assert.assertArrayEquals;

@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class MatcherBenchmarks extends Compiler2TestBase {


	@Test
	public void test0_addImmediate() {
		TimingResults tr = new TimingResults();
		testResultEqual("addImmediate", "(I)I", tr, 100000000);
		tr.report();
	}

	@Test
	public void test1_mulImmediate() {
		TimingResults tr = new TimingResults();
		testResultEqual("mulImmediate", "(I)I", tr, 100000000);
		tr.report();
	}

	@Test
	public void test2_baseIndex_Displacement() {
		TimingResults tr = new TimingResults();
		testResultEqual("baseIndex_Displacement", "(III)I", tr, 100000000, 123, 321);
		tr.report();
	}

	@Test
	public void test3_base_IndexDisplacement() {
		TimingResults tr = new TimingResults();
		testResultEqual("base_IndexDisplacement", "(III)I", tr, 100000000, 123, 321);
		tr.report();
	}

	@Test
	public void test4_base_IndexScale() {
		TimingResults tr = new TimingResults();
		testResultEqual("base_IndexScale", "(III)I", tr, 100000000, 123, 321);
		tr.report();
	}

	@Test
	public void test5_indexScale_Base() {
		TimingResults tr = new TimingResults();
		testResultEqual("indexScale_Base", "(III)I", tr, 100000000, 123, 321);
		tr.report();
	}

	@Test
	public void test6_base_indexScale__Displacement() {
		TimingResults tr = new TimingResults();
		testResultEqual("base_indexScale__Displacement", "(III)I", tr, 100000000, 123, 321);
		tr.report();
	}

	@Test
	public void test7_base__indexScale_Displacement() {
		TimingResults tr = new TimingResults();
		testResultEqual("base__indexScale_Displacement", "(III)I", tr, 100000000, 123, 321);
		tr.report();
	}


	@Test @Ignore
	public void test8_conv() throws Throwable {
		TimingResults tr = new TimingResults();

		Random rand = new Random(42);
		int n=137, m=78, p=89;

		int[][] a = new int[n][m];
		int[][] b = new int[m][p];
		int[][] c = new int[a.length][b[0].length];

		for (int i=0; i<n; ++i){
			for (int j=0; j<m; ++j){
				a[i][j] = rand.nextInt(1000);
			}
		}

		for (int i=0; i<m; ++i){
			for (int j=0; j<p; ++j){
				b[i][j] = rand.nextInt(1000);
			}
		}


		int[][] aBaseline = a.clone();
		int[][] bBaseline = b.clone();
		int[][] cBaseline = c.clone();
		runBaseline("conv", "([[I[[I[[I)V", tr.baseline, aBaseline, bBaseline, cBaseline);

		int[][] aCompiler2 = a.clone();
		int[][] bCompiler2 = b.clone();
		int[][] cCompiler2 = c.clone();
		runCompiler2("conv", "([[I[[I[[I)V", tr.compiler2, aCompiler2, bCompiler2, cCompiler2);



		assertArrayEquals(aBaseline, aCompiler2);
		assertArrayEquals(bBaseline, bCompiler2);
		assertArrayEquals(cBaseline, cCompiler2);

		assertNotEquals(aBaseline, aCompiler2);
		assertNotEquals(bBaseline, bCompiler2);
		assertNotEquals(cBaseline, cCompiler2);
		
		tr.report();
	}

	@Test
	public void test9_matTrans() throws Throwable {
		TimingResults tr = new TimingResults();

		Random rand = new Random(42);
		int n=97, m=57, p=117;

		int[][] a = new int[n][m];
		int[][] b = new int[m][p];
		int[][] c = new int[a.length][b[0].length];

		for (int i=0; i<n; ++i){
			for (int j=0; j<m; ++j){
				a[i][j] = rand.nextInt(1000);
			}
		}

		for (int i=0; i<m; ++i){
			for (int j=0; j<p; ++j){
				b[i][j] = rand.nextInt(1000);
			}
		}


		int[][] aBaseline = a.clone();
		int[][] bBaseline = b.clone();
		int[][] cBaseline = c.clone();
		runBaseline("matTrans", "([[I[[I[[I)V", tr.baseline, aBaseline, bBaseline, cBaseline);

		int[][] aCompiler2 = a.clone();
		int[][] bCompiler2 = b.clone();
		int[][] cCompiler2 = c.clone();
		runCompiler2("matTrans", "([[I[[I[[I)V", tr.compiler2, aCompiler2, bCompiler2, cCompiler2);


		assertArrayEquals(aBaseline, aCompiler2);
		assertArrayEquals(bBaseline, bCompiler2);
		assertArrayEquals(cBaseline, cCompiler2);

		assertNotEquals(aBaseline, aCompiler2);
		assertNotEquals(bBaseline, bCompiler2);
		assertNotEquals(cBaseline, cCompiler2);

		tr.report();
	}
	/**
	 * These methods are tested.
	 */


	static int addImmediate(int x) {
		int a = 0;
		for (int i=0; i <= x; i++){
			a = i + 23123;
			int b = a + 1;
			int c = b + 2;
			int d = c + 3;
			int e = d + 4;
			int f = e + 5;
		}
		return a;
	}

	static int mulImmediate(int x) {
		int c = 0;
		for (int i=0; i <= x; i++){
			int a = i * 3;
			int b = a * 9;
			c = b * 7;
		}
		return c;
	}


	static int baseIndex_Displacement(int count, int base, int index) {
		int a = 0;
		for (int i=0; i <= count; i++){
			base = base+i;
			index = index+i;
			a = (base + index) + 3;
		}
		return a;
	}

	static int base_IndexDisplacement(int count, int base, int index) {
		int a = 0;
		for (int i=0; i <= count; i++){
			base = base+i;
			index = index+i;
			a = base + (index + 3);
		}
		return a;
	}

	static int base_IndexScale(int count, int base, int index) {
		int a = 0;
		for (int i=0; i <= count; i++){
			base = base+i;
			index = index+i;
			a = base + (index * 2);
		}
		return a;
	}

	static int indexScale_Base(int count, int base, int index) {
		int a = 0;
		for (int i=0; i <= count; i++){
			base = base+i;
			index = index+i;
			a = (index * 2) + base;
		}
		return a;
	}

	static int base_indexScale__Displacement(int count, int base, int index) {
		int a = 0;
		for (int i=0; i <= count; i++){
			base = base+i;
			index = index+i;
			a = (base + (index * 2)) + 3;
		}
		return a;
	}

	static int base__indexScale_Displacement(int count, int base, int index) {
		int a = 0;
		for (int i=0; i <= count; i++){
			base = base+i;
			index = index+i;
			a = base + ((index * 4) + 3);
		}
		return a;
	}

	static void conv(int A[][], int B[][], int AB[][]) {
	  // sanity checks
	  int n = A.length;
	  int m = B.length;
	  if (n == 0 || m == 0) return;
	  if (A[0].length != m) return;
	  int p = B[0].length;
	  if (AB.length != n) return;
	  if (AB[0].length != p) return;

	  for (int i = 0; i < n; ++i) {
	    for (int j = 0; j < p; ++j) {
	      int sum=0;
	      for (int k = 0; k < m; ++k) {
	        sum += A[i][k] * B[k][j];
	      }
	      AB[i][j] = sum;
	    }
	  }
	  for (int i = 0; i < n; ++i) {
	    for (int j = 0; j < p; ++j) {
	      int sum=0;
	      for (int k = 0; k < m; ++k) {
	        sum += A[i][k] * B[k][j];
	      }
	      AB[i][j] += sum;
	    }
	  }
	}
	
	static void matTrans(int A[][], int B[][], int AB[][]) {
	  // sanity checks
	  int n = A.length;
	  int m = B.length;
	  if (n == 0 || m == 0) return;
	  if (A[0].length != m) return;
	  int p = B[0].length;
	  if (AB.length != n) return;
	  if (AB[0].length != p) return;

	  for (int i = 0; i < n; ++i) {
	    for (int j = 0; j < p; ++j) {
	      int sum=0;
	      for (int k = 0; k < m; ++k) {
	        sum += A[i][k] * B[k][j];
	      }
	      AB[i][j] = sum;
	    }
	    for (int j = 0; j < p; ++j) {
	      int sum=0;
	      for (int k = 0; k < m; ++k) {
	        sum += A[i][k] * B[k][j];
	      }
	      AB[i][j] += sum;
	    }
	  }
	}
}
