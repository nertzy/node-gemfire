package benchmark;

import com.gemstone.gemfire.cache.*;
import com.gemstone.gemfire.cache.client.ClientCache;
import com.gemstone.gemfire.cache.client.ClientCacheFactory;
import com.gemstone.gemfire.pdx.JSONFormatter;
import com.gemstone.gemfire.pdx.PdxInstance;
import org.apache.commons.lang3.RandomStringUtils;
import org.apache.commons.math3.stat.descriptive.DescriptiveStatistics;

import java.io.File;
import org.apache.commons.io.FileUtils;
import java.util.ArrayList;
import java.util.HashMap;

public class Benchmark {
    public static final int NANOSECONDS_IN_A_MILLISECOND = 1000 * 1000;
    static int KEY_SIZE = 8;
    static int VALUE_SIZE = 15 * 1024;
    static int NUMBER_OF_ITEMS = 10000;
    private static String randomObject;

    public static void main(String[] args){

        ClientCache cache = new ClientCacheFactory()
                .set("log-level", "warning")
                .set("name", "BenchmarkClient")
                .set("cache-xml-file", "../xml/BenchmarkClient.xml")
                .create();

        Region region = cache.getRegion("exampleRegion");

        region.put("smoke", "test");
        if(!region.get("smoke").equals("test")) {
            throw new RuntimeException("Smoke test failed");
        }

        try {
            randomObject = FileUtils.readFileToString(new File("../data/randomObject.json"));
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        System.out.println("Generating data...");
        ArrayList<String> keys = generateKeys();

        System.out.println("Warming up the JVM...");
        //warm up the JVM
        benchmarkPut(region, keys);
        benchmarkGet(region, keys);
        region.clear();

        System.out.println("Benchmarking put...");
        ArrayList<Double> putResults = benchmarkPut(region, keys);
        System.out.println("Benchmarking get...");
        ArrayList<Double> getResults = benchmarkGet(region, keys);

        writeData("Put", putResults);
        writeData("Get", getResults);

        cache.close();
    }

    private static ArrayList<String> generateKeys() {
        ArrayList<String> keys = new ArrayList<String>();

        for(int i = 0; i < NUMBER_OF_ITEMS; i++) {
            keys.add(RandomStringUtils.randomAlphanumeric(KEY_SIZE));
        }

        return keys;
    }

    private static ArrayList<Double> benchmarkPut(Region region, ArrayList<String> keys) {
        ArrayList<Double> results = new ArrayList<Double>();
        Long lastNanoTime;

        for(Object key : keys) {
            lastNanoTime = System.nanoTime();

            String json = randomObject;
            PdxInstance pdxInstance = JSONFormatter.fromJSON(json);

            region.put(key, pdxInstance);

            results.add((double)(System.nanoTime() - lastNanoTime));
        }

        return results;
    }

    private static ArrayList<Double> benchmarkGet(Region region, ArrayList<String> keys) {
        ArrayList<Double> results = new ArrayList<Double>();
        Long lastNanoTime;

        for(Object key : keys) {
            lastNanoTime = System.nanoTime();
            JSONFormatter.toJSON((PdxInstance) region.get(key));
            results.add((double)(System.nanoTime() - lastNanoTime));
        }

        return results;
    }

    private static void writeData(String description, ArrayList<Double> results) {
        double[] unboxedResults = new double[NUMBER_OF_ITEMS];
        for(int i = 0; i < NUMBER_OF_ITEMS; i++) {
            unboxedResults[i] = results.get(i);
        }

        DescriptiveStatistics descriptiveStatistics = new DescriptiveStatistics(unboxedResults);

        double mean = descriptiveStatistics.getMean() / NANOSECONDS_IN_A_MILLISECOND;
        double standardDeviation = descriptiveStatistics.getStandardDeviation() / NANOSECONDS_IN_A_MILLISECOND;
        double ninetyFifthPercentile = descriptiveStatistics.getPercentile(95) / NANOSECONDS_IN_A_MILLISECOND;

        System.out.println("\nResults for " + description + ": ");
        System.out.format("%f operations per second\n", 1000 / mean);
        System.out.format("Mean duration: %fms\n", mean);
        System.out.format("Std dev: %fms\n", standardDeviation);
        System.out.format("95th percentile: %fms\n", ninetyFifthPercentile);
    }
}
