package benchmark;

import com.gemstone.gemfire.cache.*;
import com.gemstone.gemfire.cache.client.ClientCache;
import com.gemstone.gemfire.cache.client.ClientCacheFactory;
import org.apache.commons.lang3.RandomStringUtils;
import org.apache.commons.math3.stat.descriptive.DescriptiveStatistics;

import java.util.ArrayList;
import java.util.HashMap;

public class Benchmark {
    public static final int NANOSECONDS_IN_A_MILLISECOND = 1000 * 1000;
    static int KEY_SIZE = 8;
    static int VALUE_SIZE = 15 * 1024;
    static int NUMBER_OF_ITEMS = 2000;

    public static void main(String[] args){

        ClientCache cache = new ClientCacheFactory()
                .set("log-level", "error")
                .set("name", "BenchmarkClient")
                .set("cache-xml-file", "xml/BenchmarkClient.xml")
                .create();

        Region region = cache.getRegion("exampleRegion");

        region.put("smoke", "test");
        if(!region.get("smoke").equals("test")) {
            throw new RuntimeException("Smoke test failed");
        }

        HashMap<String,String> data = generateData();

        ArrayList<Double> putResults = benchmarkPut(region, data);
        ArrayList<Double> getResults = benchmarkGet(region, data);

        writeData("Put", putResults);
        writeData("Get", getResults);

        cache.close();
    }

    private static HashMap<String, String> generateData() {
        HashMap<String,String> data = new HashMap<String,String>();

        for(int i = 0; i < NUMBER_OF_ITEMS; i++) {
            String key = RandomStringUtils.randomAlphanumeric(KEY_SIZE);
            String value = RandomStringUtils.randomAscii(VALUE_SIZE);
            data.put(key, value);
        }

        return data;
    }

    private static ArrayList<Double> benchmarkPut(Region region, HashMap<String,String> data) {
        ArrayList<Double> results = new ArrayList<Double>();
        Object[] keys = data.keySet().toArray();
        Long lastNanoTime;

        for(Object key : keys) {
            lastNanoTime = System.nanoTime();
            region.put(key, data.get(key));
            results.add((double)(System.nanoTime() - lastNanoTime));
        }

        return results;
    }

    private static ArrayList<Double> benchmarkGet(Region region, HashMap<String, String> data) {
        ArrayList<Double> results = new ArrayList<Double>();
        Object[] keys = data.keySet().toArray();
        Long lastNanoTime;

        for(Object key : keys) {
            lastNanoTime = System.nanoTime();
            region.get(key);
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
