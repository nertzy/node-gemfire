package io.pivotal.node_gemfire;

import com.gemstone.gemfire.cache.Cache;
import com.gemstone.gemfire.cache.CacheFactory;
import com.gemstone.gemfire.cache.execute.FunctionAdapter;
import com.gemstone.gemfire.cache.execute.FunctionContext;
import com.gemstone.gemfire.pdx.PdxInstanceFactory;

public class ReturnObjectArrayTriggersError extends FunctionAdapter {

    public void execute(FunctionContext fc) {
        Object[] objectArray = new Object[] {
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000",
                "000000000000000000000000"};

        Cache cache = CacheFactory.getAnyInstance();
        PdxInstanceFactory pdxInstanceFactory = cache.createPdxInstanceFactory("JSON object");
        pdxInstanceFactory.writeObjectArray("someArray", objectArray);

        fc.getResultSender().lastResult(pdxInstanceFactory.create());
    }

    public String getId() {
        return getClass().getName();
    }

}
