package benchmark;

import com.gemstone.gemfire.cache.*;
import com.gemstone.gemfire.cache.client.ClientCache;
import com.gemstone.gemfire.cache.client.ClientCacheFactory;
import com.gemstone.gemfire.cache.query.*;
import com.gemstone.gemfire.pdx.JSONFormatter;
import com.gemstone.gemfire.pdx.PdxInstance;
import org.apache.commons.lang3.RandomStringUtils;
import org.apache.commons.math3.stat.descriptive.DescriptiveStatistics;

import java.io.File;
import org.apache.commons.io.FileUtils;
import java.util.ArrayList;

public class Benchmark {
    public static final int NANOSECONDS_IN_A_MILLISECOND = 1000 * 1000;
    private static final int QUERY_COUNT = 10;
    private static final int MICROSECONDS_IN_A_MILLISECOND = 1000;
    private static final int MILLISECONDS_IN_A_SECOND = 1000;
    static int KEY_SIZE = 8;
    static int VALUE_SIZE = 15 * 1024;
    static int NUMBER_OF_ITEMS = 10000;

    public static void main(String[] args) throws NameResolutionException, TypeMismatchException, QueryInvocationTargetException, FunctionDomainException {

        ClientCache cache = new ClientCacheFactory()
                .set("log-level", "warning")
                .set("name", "BenchmarkClient")
                .set("cache-xml-file", "../../xml/ExampleClient.xml")
                .create();

        Region region = cache.getRegion("exampleRegion");

        region.put("smoke", "test");
        if(!region.get("smoke").equals("test")) {
            throw new RuntimeException("Smoke test failed");
        }

        String testString = RandomStringUtils.randomAlphanumeric(VALUE_SIZE);
        String simpleObject = "{ \"foo\": \"" + testString + "\" }";
        String randomObject;
        try {
            randomObject = FileUtils.readFileToString(new File("../data/randomObject.json"));
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        System.out.println("Generating data...");
        ArrayList<String> keys = generateKeys();

        System.out.println("Warming up the JVM...");
        //warm up the JVM
        benchmarkStringPut(region, keys, testString);
        benchmarkObjectPut(region, keys, simpleObject);
        benchmarkObjectPut(region, keys, randomObject);

        ArrayList<Double> stringPutResults = benchmarkStringPut(region, keys, testString);
        writeData("String put", stringPutResults);

        ArrayList<Double> simpleObjectPutResults = benchmarkObjectPut(region, keys, simpleObject);
        writeData("Simple object put", simpleObjectPutResults);

        ArrayList<Double> complexObjectPutResults = benchmarkObjectPut(region, keys, randomObject);
        writeData("Complex object put", complexObjectPutResults);

        benchmarkOQL(cache, 100);
        benchmarkOQL(cache, 1000);
        benchmarkOQL(cache, 10000);

        cache.close();
    }

    private static ArrayList<String> generateKeys() {
        ArrayList<String> keys = new ArrayList<String>();

        for(int i = 0; i < NUMBER_OF_ITEMS; i++) {
            keys.add(RandomStringUtils.randomAlphanumeric(KEY_SIZE));
        }

        return keys;
    }

    private static ArrayList<Double> benchmarkStringPut(Region region, ArrayList<String> keys, String value) {
        ArrayList<Double> results = new ArrayList<Double>();
        Long lastNanoTime;

        region.clear();
        for(Object key : keys) {
            lastNanoTime = System.nanoTime();

            region.put(key, value);

            results.add((double)(System.nanoTime() - lastNanoTime));
        }

        return results;
    }

    private static ArrayList<Double> benchmarkObjectPut(Region region, ArrayList<String> keys, String json) {
        ArrayList<Double> results = new ArrayList<Double>();
        Long lastNanoTime;

        region.clear();
        for(Object key : keys) {
            lastNanoTime = System.nanoTime();

            PdxInstance pdxInstance = JSONFormatter.fromJSON(json);

            region.put(key, pdxInstance);

            results.add((double)(System.nanoTime() - lastNanoTime));
        }

        return results;
    }

    private static void benchmarkOQL(ClientCache cache, int n) throws NameResolutionException, TypeMismatchException, QueryInvocationTargetException, FunctionDomainException {
        String needleJSON = "{\"name\":\"Jane Doe\",\"addresses\":[{\"phoneNumbers\":[{\"number\":\"212-987-5440\"},{\"number\":\"717-734-2230\"}]},{\"city\":\"New York\"}]}";
        String haystackJSON = "{\"name\":\"Jane Doe\",\"addresses\":[{\"phoneNumbers\":[{\"number\":\"555-555-1212\"},{\"number\":\"415-77-PIVOT\"}]},{\"city\":\"New York\"}]}";
        String query = "SELECT person.name " +
                "FROM " +
                "(SELECT * FROM /oqlBenchmark jr " +
                " WHERE is_defined(jr.addresses)) person, " +
                "(SELECT * FROM person.addresses) a " +
                "WHERE is_defined(a.phoneNumbers) " +
                "AND '212-987-5440' IN ( " +
                "SELECT n.number FROM a.phoneNumbers n)";

        Region region = cache.getRegion("oqlBenchmark");
        region.clear();
        region.put("needle", JSONFormatter.fromJSON(needleJSON));
        for(int i = 0; i < n - 1; i++) {
            region.put("haystack_" + i, JSONFormatter.fromJSON(haystackJSON));
        }


        ArrayList<Double> results = new ArrayList<Double>();
        Long lastNanoTime;

        for(int i = 0; i < QUERY_COUNT; i++) {
            lastNanoTime = System.nanoTime();

            region.getRegionService().getQueryService().newQuery(query).execute();

            results.add((double)(System.nanoTime() - lastNanoTime));
        }

        writeData("OQL (" + n + " entries)", results);
    }

    private static void writeData(String description, ArrayList<Double> results) {
        int numberOfItems = results.size();
        double[] unboxedResults = new double[numberOfItems];
        for(int i = 0; i < numberOfItems; i++) {
            unboxedResults[i] = results.get(i);
        }

        DescriptiveStatistics descriptiveStatistics = new DescriptiveStatistics(unboxedResults);

        double meanMillis = descriptiveStatistics.getMean() / NANOSECONDS_IN_A_MILLISECOND;
        double standardDeviationMillis = descriptiveStatistics.getStandardDeviation() / NANOSECONDS_IN_A_MILLISECOND;

        System.out.format("%s: %d (± %d) usec/op %d ops/sec\n",
                description,
                Math.round(meanMillis * MICROSECONDS_IN_A_MILLISECOND),
                Math.round(standardDeviationMillis * MICROSECONDS_IN_A_MILLISECOND),
                Math.round(MILLISECONDS_IN_A_SECOND/meanMillis));
    }
}
