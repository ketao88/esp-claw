import { createEffect, createSignal, For, Show, type Component } from 'solid-js';
import { currentLocale, t, tf } from '../i18n';
import { fetchWifiScan } from '../api/client';
import type { AppConfig, WifiApInfo } from '../api/client';
import { createConfigTab } from '../state/configTab';
import { appStatus } from '../state/config';
import { TabShell } from '../components/layout/TabShell';
import { PageHeader } from '../components/ui/PageHeader';
import { CollapsibleConfigBlock, StaticConfigBlock } from '../components/ui/ConfigBlocks';
import { TextInput, SelectInput } from '../components/ui/FormField';
import { Button } from '../components/ui/Button';
import { SavePanel } from '../components/ui/SavePanel';
import { Banner } from '../components/ui/Banner';
import { RestartConfirmModal } from '../components/system/RestartConfirmModal';
import { pushToast } from '../state/toast';

type BasicForm = {
  wifi_ssid: string;
  wifi_password: string;
  ap_ssid: string;
  ap_password: string;
  ap_behavior: string;
  time_timezone: string;
};

export const BasicPage: Component<{ onRestartRequest: () => void }> = (props) => {
  const tab = createConfigTab<BasicForm>({
    tab: 'basic',
    groups: ['wifi', 'time'],
    toForm: (config: Partial<AppConfig>) => ({
      wifi_ssid: config.wifi_ssid ?? '',
      wifi_password: config.wifi_password ?? '',
      ap_ssid: config.ap_ssid ?? '',
      ap_password: config.ap_password ?? '',
      ap_behavior: config.ap_behavior ?? 'keep',
      time_timezone: config.time_timezone ?? '',
    }),
    fromForm: (form) => ({
      wifi_ssid: form.wifi_ssid.trim(),
      wifi_password: form.wifi_password,
      ap_ssid: form.ap_ssid.trim(),
      ap_password: form.ap_password,
      ap_behavior: form.ap_behavior,
      time_timezone: form.time_timezone.trim(),
    }),
  });
  const [validationError, setValidationError] = createSignal<string | null>(null);
  const [confirmOpen, setConfirmOpen] = createSignal(false);

  /* Wi-Fi scan state */
  const [scanning, setScanning] = createSignal(false);
  const [scanResults, setScanResults] = createSignal<WifiApInfo[]>([]);
  const [showScanDropdown, setShowScanDropdown] = createSignal(false);
  const [scanError, setScanError] = createSignal<string | null>(null);

  createEffect(() => {
    void tab.form.wifi_ssid;
    void tab.form.wifi_password;
    void tab.form.ap_ssid;
    void tab.form.ap_password;
    setValidationError(null);
  });

  const doScan = async () => {
    setScanning(true);
    setScanError(null);
    try {
      const result = await fetchWifiScan();
      if (result.ok && Array.isArray(result.aps)) {
        setScanResults(result.aps);
        setShowScanDropdown(true);
        if (result.aps.length === 0) {
          pushToast(t('wifiScanNoAps') as string, 'info', 4000);
        } else {
          pushToast(tf('wifiScanRefreshed', { count: result.aps.length }), 'success', 3000);
        }
      } else {
        setScanError(result.message ?? (t('wifiScanFailed') as string));
      }
    } catch (err) {
      setScanError((err as Error).message ?? (t('wifiScanFailed') as string));
    } finally {
      setScanning(false);
    }
  };

  const selectSsid = (ssid: string) => {
    tab.setForm('wifi_ssid', ssid);
    setShowScanDropdown(false);
  };

  const handleSave = async () => {
    const wifiSsid = tab.form.wifi_ssid.trim();
    const wifiPassword = tab.form.wifi_password;
    const apPassword = tab.form.ap_password;

    if (!wifiSsid) {
      const message = t('wifiValidationSsidRequired') as string;
      setValidationError(message);
      pushToast(message, 'error', 5000);
      return;
    }

    if (wifiPassword.length > 0 && wifiPassword.length < 8) {
      const message = t('wifiValidationPasswordLength') as string;
      setValidationError(message);
      pushToast(message, 'error', 5000);
      return;
    }

    if (apPassword.length > 0 && apPassword.length < 8) {
      const message = t('apValidationPasswordLength') as string;
      setValidationError(message);
      pushToast(message, 'error', 5000);
      return;
    }

    await tab.save();
    setConfirmOpen(true);
  };

  const currentApSsid = () => appStatus()?.ap_ssid ?? '';

  const apNameHint = () => {
    const ssid = currentApSsid();
    return ssid ? tf('apNameHint', { ssid }) : '';
  };

  const timezoneHint = () =>
    currentLocale() === 'zh-cn' ? (
      <>
        仅接受 POSIX TZ 字符串，符号与日常 UTC 表示相反。北京时间（UTC+8）应写作
        {' '}
        &quot;CST-8&quot;
        ，纽约（UTC-5）写作 &quot;EST5&quot;。可在
        <a
          href="https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv"
          target="_blank"
          rel="noopener noreferrer"
          class="underline underline-offset-2 hover:text-[var(--color-text-primary)]"
        >
          此表格
        </a>
        查阅 IANA 时区与 POSIX 表达转换关系。
      </>
    ) : (
      (t('timezoneHelp') as string)
    );

  return (
    <TabShell>
      <PageHeader
        title={t('navBasic') as string}
        description={t('restartHint') as string}
      />
      <Show when={validationError() ?? tab.error()}>
        <div class="px-5 pt-4">
          <Banner kind="error" message={validationError() ?? tab.error() ?? undefined} />
        </div>
      </Show>
      <div class="divide-y divide-[var(--color-border-subtle)] mt-2">
        <StaticConfigBlock title={t('sectionWifi') as string}>
          <div class="grid gap-3 sm:grid-cols-2 pt-2">
            {/* Wi-Fi SSID with scan button */}
            <div class="flex flex-col gap-1.5 sm:col-span-2">
              <label class="text-[0.8rem] text-[var(--color-text-secondary)] font-medium">
                {t('wifiSsid') as string}
              </label>
              <div class="flex gap-2 items-start">
                <div class="flex-1 relative">
                  <input
                    type="text"
                    autocomplete="off"
                    value={tab.form.wifi_ssid}
                    onInput={(e) => {
                      tab.setForm('wifi_ssid', e.currentTarget.value);
                      setShowScanDropdown(false);
                    }}
                    class="w-full rounded-[var(--radius-sm)] border border-[var(--color-border-subtle)] bg-[var(--color-bg-input)] px-3 py-2 text-sm text-[var(--color-text-primary)] transition focus:border-[rgba(232,54,45,0.4)]"
                  />
                  {/* Scan results dropdown */}
                  <Show when={showScanDropdown() && scanResults().length > 0}>
                    <div class="absolute z-50 mt-1 w-full max-h-48 overflow-y-auto rounded-[var(--radius-sm)] border border-[var(--color-border-subtle)] bg-[var(--color-bg-card)] shadow-lg">
                      <div class="sticky top-0 bg-[var(--color-bg-card)] px-3 py-1.5 text-[0.72rem] text-[var(--color-text-muted)] border-b border-[var(--color-border-subtle)] flex items-center justify-between">
                        <span>{t('wifiSelectSsid') as string}</span>
                        <button
                          type="button"
                          class="text-[var(--color-accent-soft)] hover:underline text-[0.7rem]"
                          onClick={() => doScan().catch(() => undefined)}
                        >
                          {t('wifiScanRefresh') as string}
                        </button>
                      </div>
                      <For each={scanResults()}>
                        {(ap) => (
                          <button
                            type="button"
                            class="w-full text-left px-3 py-2 text-sm text-[var(--color-text-primary)] hover:bg-white/[0.04] border-b border-[var(--color-border-subtle)] last:border-b-0 transition flex items-center gap-2"
                            onClick={() => selectSsid(ap.ssid)}
                          >
                            <svg
                              xmlns="http://www.w3.org/2000/svg"
                              viewBox="0 0 24 24"
                              fill="none"
                              stroke="currentColor"
                              stroke-width="1.5"
                              stroke-linecap="round"
                              stroke-linejoin="round"
                              class="h-3.5 w-3.5 shrink-0 text-[var(--color-text-muted)]"
                            >
                              <path d="M5 12.55a11 11 0 0 1 14.08 0" />
                              <path d="M1.42 9a16 16 0 0 1 21.16 0" />
                              <path d="M8.53 16.11a6 6 0 0 1 6.95 0" />
                              <circle cx="12" cy="20" r="1" />
                            </svg>
                            <span class="flex-1 truncate">{ap.ssid}</span>
                            <span class="shrink-0 text-[0.72rem] text-[var(--color-text-muted)] font-mono">
                              {ap.rssi}dBm
                            </span>
                            <span class="shrink-0 text-[0.7rem] text-[var(--color-text-muted)]">
                              {ap.auth.replace(/_/g, ' ')}
                            </span>
                            <span class="shrink-0 text-[0.7rem] text-[var(--color-text-muted)]">
                              ch{ap.channel}
                            </span>
                          </button>
                        )}
                      </For>
                    </div>
                  </Show>
                </div>
                <Button
                  size="sm"
                  variant="secondary"
                  onClick={() => doScan().catch(() => undefined)}
                  disabled={scanning()}
                  class="shrink-0"
                >
                  <Show
                    when={!scanning()}
                    fallback={
                      <>
                        <svg class="animate-spin h-3 w-3" viewBox="0 0 24 24" fill="none">
                          <circle class="opacity-25" cx="12" cy="12" r="10" stroke="currentColor" stroke-width="4" />
                          <path class="opacity-75" fill="currentColor" d="M4 12a8 8 0 018-8v4a4 4 0 00-4 4H4z" />
                        </svg>
                        {t('wifiScanning') as string}
                      </>
                    }
                  >
                    <svg
                      xmlns="http://www.w3.org/2000/svg"
                      viewBox="0 0 24 24"
                      fill="none"
                      stroke="currentColor"
                      stroke-width="1.8"
                      stroke-linecap="round"
                      stroke-linejoin="round"
                      class="h-3.5 w-3.5"
                    >
                      <path d="M5 12.55a11 11 0 0 1 14.08 0" />
                      <path d="M1.42 9a16 16 0 0 1 21.16 0" />
                      <path d="M8.53 16.11a6 6 0 0 1 6.95 0" />
                      <circle cx="12" cy="20" r="1" />
                    </svg>
                    {t('wifiScan') as string}
                  </Show>
                </Button>
              </div>
              <Show when={scanError()}>
                <small class="text-[0.72rem] text-[var(--color-danger)]">{scanError()}</small>
              </Show>
            </div>
            <TextInput
              type="password"
              label={t('wifiPassword')}
              autocomplete="new-password"
              value={tab.form.wifi_password}
              onInput={(event) => tab.setForm('wifi_password', event.currentTarget.value)}
            />
            <TextInput
              label={t('apName')}
              autocomplete="off"
              hint={apNameHint()}
              value={tab.form.ap_ssid}
              onInput={(event) => tab.setForm('ap_ssid', event.currentTarget.value)}
            />
            <TextInput
              type="password"
              label={t('apPassword')}
              autocomplete="new-password"
              hint={t('apPasswordHint') as string}
              value={tab.form.ap_password}
              onInput={(event) => tab.setForm('ap_password', event.currentTarget.value)}
            />
            <SelectInput
              label={t('apBehavior')}
              value={tab.form.ap_behavior}
              onChange={(event) => tab.setForm('ap_behavior', event.currentTarget.value)}
            >
              <option value="keep">{t('apBehaviorKeep') as string}</option>
              <option value="close_on_sta">{t('apBehaviorCloseOnSta') as string}</option>
            </SelectInput>
          </div>
        </StaticConfigBlock>
        <CollapsibleConfigBlock title={t('sectionAdvanced') as string} defaultOpen={false}>
          <div class="pt-2">
            <TextInput
              full
              label={t('timezone')}
              placeholder={t('timezonePlaceholder') as string}
              hint={timezoneHint()}
              value={tab.form.time_timezone}
              onInput={(event) => tab.setForm('time_timezone', event.currentTarget.value)}
            />
          </div>
        </CollapsibleConfigBlock>
      </div>
      <SavePanel
        dirty={tab.dirty()}
        saving={tab.saving()}
        onSave={() => handleSave().catch(() => undefined)}
        onDiscard={tab.discard}
        note={t('restartHint') as string}
      />
      <RestartConfirmModal
        open={confirmOpen()}
        onClose={() => setConfirmOpen(false)}
        onConfirm={() => {
          setConfirmOpen(false);
          props.onRestartRequest();
        }}
        subtitle={t('restartHint') as string}
      />
    </TabShell>
  );
};
