package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.Region;
import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;
import com.gemstone.gemfire.cache.execute.RegionFunctionContext;

import java.util.Map;
import java.util.Set;

public class Put extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        RegionFunctionContext regionFunctionContext = (RegionFunctionContext) fc;
        Region<Object, Object> region = regionFunctionContext.getDataSet();

        Object [] arguments = (Object[]) regionFunctionContext.getArguments();
        region.put(arguments[0], arguments[1]);

        fc.getResultSender().lastResult(true);
    }

    public String getId() {
        return getClass().getName();
    }
}
