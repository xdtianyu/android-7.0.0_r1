package com.android.certinstaller;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.ActivityNotFoundException;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.res.Resources;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiEnterpriseConfig;
import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.security.Credentials;
import android.security.KeyStore;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.os.AsyncTask;

import java.security.PrivateKey;
import java.security.interfaces.RSAPrivateKey;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;

public class WiFiInstaller extends Activity {

    private static final String TAG = "WifiInstaller";
    private static final String NETWORK_NAME = "network_name";
    private static final String INSTALL_STATE = "install_state";
    public static final int INSTALL_SUCCESS = 2;
    public static final int INSTALL_FAIL = 1;
    public static final int INSTALL_FAIL_NO_WIFI = 0;
    WifiConfiguration mWifiConfiguration;
    WifiManager mWifiManager;
    boolean doNotInstall;

    @Override
    protected void onCreate(Bundle savedStates) {
        super.onCreate(savedStates);

        Bundle bundle = getIntent().getExtras();
        String uriString = bundle.getString(CertInstallerMain.WIFI_CONFIG_FILE);
        String mimeType = bundle.getString(CertInstallerMain.WIFI_CONFIG);
        byte[] data = bundle.getByteArray(CertInstallerMain.WIFI_CONFIG_DATA);

        Log.d(TAG, "WiFi data for " + CertInstallerMain.WIFI_CONFIG + ": " +
                mimeType + " is " + (data != null ? data.length : "-"));

        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        mWifiConfiguration = mWifiManager.buildWifiConfig(uriString, mimeType, data);

        if (mWifiConfiguration != null) {
            WifiEnterpriseConfig enterpriseConfig = mWifiConfiguration.enterpriseConfig;
            doNotInstall = (enterpriseConfig.getEapMethod() == WifiEnterpriseConfig.Eap.TTLS
                    || enterpriseConfig.getEapMethod() == WifiEnterpriseConfig.Eap.TLS)
                    && enterpriseConfig.getCaCertificate() == null;
        } else {
            Log.w(TAG, "failed to build wifi configuration");
            doNotInstall = true;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        createMainDialog();
    }

    public static List<String> splitDomain(String domain) {
        if (domain.endsWith(".")) {
            domain = domain.substring(0, domain.length() - 1);
        }

        String[] labels = domain.toLowerCase().split("\\.");
        LinkedList<String> labelList = new LinkedList<>();
        for (String label : labels) {
            labelList.addFirst(label);
        }

        return labelList;
    }

    public static boolean sameBaseDomain(List<String> arg1, String domain) {
        if (domain == null) {
            return false;
        }

        List<String> arg2 = splitDomain(domain);
        if (arg2.isEmpty()) {
            return false;
        }
        Iterator<String> l1 = arg1.iterator();
        Iterator<String> l2 = arg2.iterator();

        while(l1.hasNext() && l2.hasNext()) {
            if (!l1.next().equals(l2.next())) {
                return false;
            }
        }
        return true;
    }

    private void createMainDialog() {
        Resources res = getResources();
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        View layout = getLayoutInflater().inflate(R.layout.wifi_main_dialog, null);
        builder.setView(layout);

        TextView text = (TextView) layout.findViewById(R.id.wifi_info);
        if (!doNotInstall) {
            text.setText(String.format(getResources().getString(R.string.wifi_installer_detail),
                    mWifiConfiguration.providerFriendlyName));

            builder.setTitle(mWifiConfiguration.providerFriendlyName);
            builder.setIcon(res.getDrawable(R.drawable.signal_wifi_4_bar_lock_black_24dp));

            builder.setPositiveButton(R.string.wifi_install_label,
                    new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    final boolean wifiEnabled = mWifiManager.isWifiEnabled();
                    if (wifiEnabled) {
                        Toast.makeText(WiFiInstaller.this, getString(R.string.wifi_installing_label),
                                Toast.LENGTH_LONG).show();
                    }
                    AsyncTask.execute(new Runnable() {
                        @Override
                        public void run() {
                            boolean success = false;
                            if (wifiEnabled) {
                                List<String> newDomain = splitDomain(mWifiConfiguration.FQDN);
                                for (WifiConfiguration config :
                                        mWifiManager.getConfiguredNetworks()) {
                                    if (sameBaseDomain(newDomain, config.FQDN)) {
                                        mWifiManager.removeNetwork(config.networkId);
                                        break;
                                    }
                                }
                                try {
                                    success = mWifiManager.addNetwork(mWifiConfiguration) != -1
                                            && mWifiManager.saveConfiguration();
                                }
                                catch (RuntimeException rte) {
                                    Log.w(TAG, "Caught exception while installing wifi config: " +
                                            rte, rte);
                                    success = false;
                                }
                            }
                            if (success) {
                                Intent intent = new Intent(getApplicationContext(),
                                        CredentialsInstallDialog.class);
                                intent.putExtra(NETWORK_NAME,
                                        mWifiConfiguration.providerFriendlyName);
                                intent.putExtra(INSTALL_STATE, INSTALL_SUCCESS);
                                startActivity(intent);
                            } else {
                                Intent intent = new Intent(getApplicationContext(),
                                        CredentialsInstallDialog.class);
                                if (!wifiEnabled) {
                                    intent.putExtra(INSTALL_STATE, INSTALL_FAIL_NO_WIFI);
                                } else {
                                    intent.putExtra(INSTALL_STATE, INSTALL_FAIL);
                                }
                                startActivity(intent);
                            }
                            finish();
                        }
                    });
                    dialog.dismiss();
                }
            });

            builder.setNegativeButton(R.string.wifi_cancel_label, new
                    DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                    finish();
                }
            });
        } else {
            text.setText(getResources().getString(R.string.wifi_installer_download_error));
            builder.setPositiveButton(R.string.done_label, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                    finish();
                }
            });
        }
        builder.create().show();
    }
}
