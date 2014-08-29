package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.Region;
import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;
import com.gemstone.gemfire.cache.execute.RegionFunctionContext;

import java.util.Map;
import java.util.Set;

public class SumRegion extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        RegionFunctionContext regionFunctionContext = (RegionFunctionContext) fc;
        Region<Object, Object> dataSet = regionFunctionContext.getDataSet();

        Set<Map.Entry<Object, Object>> entries = dataSet.entrySet();

        Double sum = 0.0;
        for(Map.Entry<Object, Object> entry : entries) {
            sum += (Double) entry.getValue();
        }

        fc.getResultSender().lastResult(sum);
    }

    public String getId() {
        return getClass().getName();
    }
}
