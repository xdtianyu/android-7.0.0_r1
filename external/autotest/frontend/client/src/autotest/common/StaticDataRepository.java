package autotest.common;


import com.google.gwt.json.client.JSONObject;
import com.google.gwt.json.client.JSONValue;
import com.google.gwt.json.client.JSONArray;

import java.util.HashMap;

/**
 * A singleton class to manage a set of static data, such as the list of users.
 * The data will most likely be retrieved once at the beginning of program
 * execution.  Other classes can then retrieve the data from this shared
 * storage.
 */
public class StaticDataRepository {
    public interface FinishedCallback {
        public void onFinished();
    }
    // singleton
    public static final StaticDataRepository theInstance = new StaticDataRepository();
    
    protected JSONObject dataObject = null;
    protected HashMap<Double, String> priorityMap = null;
    
    private StaticDataRepository() {}
    
    public static StaticDataRepository getRepository() {
        return theInstance;
    }
    
    /**
     * Update the local copy of the static data from the server.
     * @param finished callback to be notified once data has been retrieved
     */
    public void refresh(final FinishedCallback finished) {
        JsonRpcProxy.getProxy().rpcCall("get_static_data", null, 
                                        new JsonRpcCallback() {
            @Override
            public void onSuccess(JSONValue result) {
                dataObject = result.isObject();
                priorityMap = new HashMap<Double, String>();
                populatePriorities(dataObject.get("priorities").isArray());
                finished.onFinished();
            }
        });
    }
    
    private void populatePriorities(JSONArray priorities) {
        for(int i = 0; i < priorities.size(); i++) {
            JSONArray priorityData = priorities.get(i).isArray();
            String priority = priorityData.get(1).isString().stringValue();
            Double priority_value = priorityData.get(0).isNumber().getValue();
            priorityMap.put(priority_value, priority);
        }
    }

    /**
     * Get a value from the static data object.
     */
    public JSONValue getData(String key) {
        return dataObject.get(key);
    }
    
    /**
     * Set a value in the repository.
     */
    public void setData(String key, JSONValue data) {
        dataObject.put(key, data);
    }
    
    public String getCurrentUserLogin() {
        return Utils.jsonToString(dataObject.get("current_user").isObject().get("login"));
    }

    public String getPriorityName(Double value) {
        if (priorityMap == null) {
            return "Unknown";
        }

        String priorityName = priorityMap.get(value);
        if (priorityName == null) {
            priorityName = value.toString();
        }

        return priorityName;
    }
}
