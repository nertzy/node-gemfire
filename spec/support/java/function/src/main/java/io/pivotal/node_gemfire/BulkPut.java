package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.Region;
import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;
import com.gemstone.gemfire.cache.execute.RegionFunctionContext;

public class BulkPut extends FunctionAdapter {

    private static final int KEY_SIZE = 32;

    public void execute(FunctionContext fc) {
        RegionFunctionContext regionFunctionContext = (RegionFunctionContext) fc;
        Region<Object, Object> region = regionFunctionContext.getDataSet();

        Object[] arguments = (Object[]) fc.getArguments();

        Object objectToPut = arguments[0];
        int numberOfPuts = ((Double) arguments[1]).intValue();

        for (int i = 0; i < numberOfPuts; i++) {
           String key = "foo" + i;
           region.put(key, objectToPut);
        }

        fc.getResultSender().lastResult(true);
    }

    public String getId() {
        return getClass().getName();
    }
}
