#include <stdio.h>
#include "Ethernet_lib.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "esp_eth_driver.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

static const char *TAG = "example_eth_init";

#define SPI_ETHERNETS_NUM 1
#define ETH_SPI_HOST 1
#define SPI_FREQUENCE 36
#define CONFIG_ETH_SPI_PHY_ADDR1 1
#define CONFIG_ETH_SPI_PHY_RST1_GPIO -1
#define CONFIG_ETH_SPI_CS1_GPIO 34
#define CONFIG_ETH_SPI_INT1_GPIO 5
#define CONFIG_ETH_SPI_ETHERNET_W5500 1

#define INIT_SPI_ETH_MODULE_CONFIG(eth_module_config, num)                    \
    do                                                                        \
    {                                                                         \
        eth_module_config[num].spi_cs_gpio = CONFIG_ETH_SPI_CS1_GPIO;         \
        eth_module_config[num].int_gpio = CONFIG_ETH_SPI_INT1_GPIO;           \
        eth_module_config[num].phy_reset_gpio = CONFIG_ETH_SPI_PHY_RST1_GPIO; \
        eth_module_config[num].phy_addr = CONFIG_ETH_SPI_PHY_ADDR1;           \
    } while (0)

typedef struct
{
    uint8_t spi_cs_gpio;
    uint8_t int_gpio;
    int8_t phy_reset_gpio;
    uint8_t phy_addr;
    uint8_t *mac_addr;
} spi_eth_module_config_t;

/**
 * @brief SPI bus initialization (to be used by Ethernet SPI modules)
 *
 * @return
 *          - ESP_OK on success
 */
static esp_err_t spi_bus_init(void)
{
    esp_err_t ret = ESP_OK;

    // Install GPIO ISR handler to be able to service SPI Eth modules interrupts
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK)
    {
        if (ret == ESP_ERR_INVALID_STATE)
        {
            ESP_LOGW(TAG, "GPIO ISR handler has been already installed");
            ret = ESP_OK; // ISR handler has been already installed so no issues
        }
        else
        {
            ESP_LOGE(TAG, "GPIO ISR handler install failed");
            goto err;
        }
    }

    // Init SPI bus
    spi_bus_config_t buscfg = {
        .miso_io_num = 37,
        .mosi_io_num = 35,
        .sclk_io_num = 36,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_GOTO_ON_ERROR(spi_bus_initialize(ETH_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO),
                      err, TAG, "SPI host #%d init failed", ETH_SPI_HOST);

err:
    return ret;
}

/**
 * @brief Ethernet SPI modules initialization
 *
 * @param[in] spi_eth_module_config specific SPI Ethernet module configuration
 * @param[out] mac_out optionally returns Ethernet MAC object
 * @param[out] phy_out optionally returns Ethernet PHY object
 * @return
 *          - esp_eth_handle_t if init succeeded
 *          - NULL if init failed
 */
static esp_eth_handle_t eth_init_spi(spi_eth_module_config_t *spi_eth_module_config, esp_eth_mac_t **mac_out, esp_eth_phy_t **phy_out)
{
    esp_eth_handle_t ret = NULL;

    // Init common MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    // Update PHY config based on board specific configuration
    phy_config.phy_addr = spi_eth_module_config->phy_addr;
    phy_config.reset_gpio_num = spi_eth_module_config->phy_reset_gpio;

    // Configure SPI interface for specific SPI module
    spi_device_interface_config_t spi_devcfg = {
        .mode = 0,
        .clock_speed_hz = SPI_FREQUENCE * 1000 * 1000,
        .queue_size = 20,
        .spics_io_num = spi_eth_module_config->spi_cs_gpio};
    // Init vendor specific MAC config to default, and create new SPI Ethernet MAC instance
    // and new PHY instance based on board configuration
    eth_w5500_config_t w5500_config = ETH_W5500_DEFAULT_CONFIG(ETH_SPI_HOST, &spi_devcfg);
    w5500_config.int_gpio_num = spi_eth_module_config->int_gpio;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&w5500_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_config);
    // Init Ethernet driver to default and install it
    esp_eth_handle_t eth_handle = NULL;
    esp_eth_config_t eth_config_spi = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_GOTO_ON_FALSE(esp_eth_driver_install(&eth_config_spi, &eth_handle) == ESP_OK, NULL, err, TAG, "SPI Ethernet driver install failed");

    // The SPI Ethernet module might not have a burned factory MAC address, we can set it manually.
    if (spi_eth_module_config->mac_addr != NULL)
    {
        ESP_GOTO_ON_FALSE(esp_eth_ioctl(eth_handle, ETH_CMD_S_MAC_ADDR, spi_eth_module_config->mac_addr) == ESP_OK,
                          NULL, err, TAG, "SPI Ethernet MAC address config failed");
    }

    if (mac_out != NULL)
    {
        *mac_out = mac;
    }
    if (phy_out != NULL)
    {
        *phy_out = phy;
    }
    return eth_handle;
err:
    if (eth_handle != NULL)
    {
        esp_eth_driver_uninstall(eth_handle);
    }
    if (mac != NULL)
    {
        mac->del(mac);
    }
    if (phy != NULL)
    {
        phy->del(phy);
    }
    return ret;
}

esp_err_t eth_init(esp_eth_handle_t *eth_handles_out[], uint8_t *eth_cnt_out)
{
    esp_err_t ret = ESP_OK;
    esp_eth_handle_t *eth_handles = NULL;
    uint8_t eth_cnt = 0;

    ESP_GOTO_ON_FALSE(eth_handles_out != NULL && eth_cnt_out != NULL, ESP_ERR_INVALID_ARG,
                      err, TAG, "invalid arguments: initialized handles array or number of interfaces");
    eth_handles = calloc(SPI_ETHERNETS_NUM, sizeof(esp_eth_handle_t));
    ESP_GOTO_ON_FALSE(eth_handles != NULL, ESP_ERR_NO_MEM, err, TAG, "no memory");

    ESP_GOTO_ON_ERROR(spi_bus_init(), err, TAG, "SPI bus init failed");
    // Init specific SPI Ethernet module configuration from Kconfig (CS GPIO, Interrupt GPIO, etc.)
    spi_eth_module_config_t spi_eth_module_config[SPI_ETHERNETS_NUM] = {0};
    INIT_SPI_ETH_MODULE_CONFIG(spi_eth_module_config, 0);
    // The SPI Ethernet module(s) might not have a burned factory MAC address, hence use manually configured address(es).
    // In this example, Locally Administered MAC address derived from ESP32x base MAC address is used.
    // Note that Locally Administered OUI range should be used only when testing on a LAN under your control!
    uint8_t base_mac_addr[ETH_ADDR_LEN];
    ESP_GOTO_ON_ERROR(esp_efuse_mac_get_default(base_mac_addr), err, TAG, "get EFUSE MAC failed");
    uint8_t local_mac_1[ETH_ADDR_LEN];
    esp_derive_local_mac(local_mac_1, base_mac_addr);
    spi_eth_module_config[0].mac_addr = local_mac_1;

    for (int i = 0; i < SPI_ETHERNETS_NUM; i++)
    {
        eth_handles[eth_cnt] = eth_init_spi(&spi_eth_module_config[i], NULL, NULL);
        ESP_GOTO_ON_FALSE(eth_handles[eth_cnt], ESP_FAIL, err, TAG, "SPI Ethernet init failed");
        eth_cnt++;
    }

    *eth_handles_out = eth_handles;
    *eth_cnt_out = eth_cnt;

    return ret;
err:
    free(eth_handles);
    return ret;
}
