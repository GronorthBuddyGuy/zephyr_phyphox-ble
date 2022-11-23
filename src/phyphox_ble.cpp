#include <cstdint>
#include <zephyr/sys/byteorder.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/crc.h>
#include <zephyr/sys/byteorder.h>
#include <phyphox_autogen.hpp>
#include <phyphox_ble.hpp>

namespace phyphox_ble
{

static constexpr uint8_t MAX_PACKET_EXP_XML{50U};
static uint16_t exp_xml_data_count{0U};
static uint16_t exp_xml_next_idx{0U};
static bool header_sent{false};

namespace event
{
    static event_cb user_cb{nullptr};
    static void * user_args{nullptr};
}

namespace exp_load
{
    static exp_load_cb user_cb{nullptr};
    static void * user_args{nullptr};
} // namespace exp_load_cb


/*!< Offsets for Phyphox raw event data*/
namespace offset
{
    static constexpr uint8_t evt = 0;
    static constexpr uint8_t exptime = 1;
    static constexpr uint8_t unixtime = 9;
}

namespace experiment
{

bool set_title(char * pData,uint8_t len)
{
    static constexpr uint16_t MAX_TITLE_LEN{EXP_TITLE_END - EXP_TITLE_START};
    if(pData!=nullptr && len<=MAX_TITLE_LEN)
    {
        memcpy(&exp_data[EXP_TITLE_START],pData,len);
        if(len<MAX_TITLE_LEN)
        {
            const uint16_t remaining = MAX_TITLE_LEN-len;
            memset(&exp_data[EXP_TITLE_START+len],(int)' ',remaining);
        }
        return true;
    }
    else
    {
        return false;
    }
}

bool set_blename_in(char * pData,uint8_t len)
{
    if(BLE_END_NAME_IN == static_cast<uint16_t>(-1))
    {
        return false;
    }
    if(BLE_END_NAME_IN<=BLE_START_NAME_IN)
    {
        return false;
    }
    static constexpr uint16_t MAX_NAME_LEN{BLE_END_NAME_IN - BLE_START_NAME_IN};
    if(pData!=nullptr && len<=MAX_NAME_LEN)
    {
        memcpy(&exp_data[BLE_START_NAME_IN],pData,len);
        if(len<MAX_NAME_LEN)
        {
            const uint16_t remaining = MAX_NAME_LEN-len;
            exp_data[BLE_START_NAME_IN+len]='"';
            if(remaining > 1)
            {
                memset(&exp_data[BLE_START_NAME_IN+len+1],(int)' ',remaining-1);
            }
            exp_data[BLE_END_NAME_IN]=' ';
            
        }
        return true;
    }
    else
    {
        return false;
    }
}

void update_crc()
{
    const uint32_t crc = crc32_ieee(exp_data,sizeof(exp_data));
    const uint32_t crc_be = sys_be32_to_cpu(crc);
    uint8_t * header_start = &exp_header[HEADER_SIZE-sizeof(uint32_t)];
    memcpy(header_start, &crc_be, sizeof(uint32_t));
}

} // namespace experiment


/*! @brief send a chunk of the complete xml to phyphox
*/
static void send_exp_xml(); 

/*! @brief Callback to sync sending the complete xml file
    over ble notification
*/
void exp_xml_notify_sent_cb(struct bt_conn *conn, void *user_data)
{
    // continue sending xml if still data pending
    if(exp_xml_data_count< phyphox_autogen::EXP_DATA_SIZE)
    {
        send_exp_xml();
    }
}

void register_expload_cb(expload_cb cb,void * args)
{
    exp_load::user_cb = cb;
    exp_load::user_args = args;
}

void register_evt_cb(event_cb cb,void * args)
{
    event::user_cb = cb;
    event::user_args = args;
}

static void exp_xml_notify(const struct bt_gatt_attr *attr, uint16_t value)
{
    const bool notify_enabled = value == BT_GATT_CCC_NOTIFY;
    if(notify_enabled)
    {
        exp_xml_data_count = 0;
        exp_xml_next_idx=0;
        header_sent = false;
        send_exp_xml();
    }
}


static ssize_t eventwrite_cb(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				const void *buf, uint16_t len, uint16_t offset,
				uint8_t flags)
{
    const auto pBufRaw = static_cast<const uint8_t*>(buf);
    /* convert big endian to little endian*/
    const uint64_t exp_time_ms =    pBufRaw[offset::exptime+7] << 56 | 
                                pBufRaw[offset::exptime+6] << 48 | 
                                pBufRaw[offset::exptime+5] << 40 | 
                                pBufRaw[offset::exptime+4] << 32 | 
                                pBufRaw[offset::exptime+3] << 24 |
                                pBufRaw[offset::exptime+2] << 16 |  
                                pBufRaw[offset::exptime+1] << 8  | 
                                pBufRaw[offset::exptime] << 16; 
    const uint64_t unixtime_ms =pBufRaw[offset::unixtime+7] << 56 | 
                                pBufRaw[offset::unixtime+6] << 48 | 
                                pBufRaw[offset::unixtime+5] << 40 | 
                                pBufRaw[offset::unixtime+4] << 32 | 
                                pBufRaw[offset::unixtime+3] << 24 |
                                pBufRaw[offset::unixtime+2] << 16 |  
                                pBufRaw[offset::unixtime+1] << 8  | 
                                pBufRaw[offset::unixtime] << 16; 

    const Event_t event_data = {.evt_type = static_cast<EventTypes>(pBufRaw[0]),
                         .exp_time_ms = exp_time_ms,
                         .unix_time_ms = unixtime_ms
                        };
    
    
    if(event::user_cb != nullptr)
    {
        event::user_cb(event_data,event::user_args);
    }
	return len;
}


constexpr uint8_t exp_xml_char_idx{1U};

BT_GATT_SERVICE_DEFINE(phy_phox_svc,
	BT_GATT_PRIMARY_SERVICE( (void *)&uuid::SERVICE), // idx 0
	BT_GATT_CHARACTERISTIC( (bt_uuid *)&uuid::charact::EXP_XML,  // idx 1
							BT_GATT_CHRC_NOTIFY,
							BT_GATT_PERM_READ,
							nullptr,
							nullptr,
							nullptr),
	BT_GATT_CCC(exp_xml_notify, // idx 2
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE), // cppcheck-suppress syntaxError
	BT_GATT_CHARACTERISTIC( (bt_uuid *)&uuid::charact::EVENT,  // idx 3
							BT_GATT_CHRC_WRITE,
							BT_GATT_PERM_WRITE,
							nullptr,
							eventwrite_cb,
							nullptr),
);


static bt_gatt_notify_params exp_notify_params =
{
    .uuid = nullptr,
    .attr = &phy_phox_svc.attrs[exp_xml_char_idx],
    .data = nullptr,
    .len = 0,
    .func = exp_xml_notify_sent_cb,
    .user_data = nullptr
};

static void send_exp_xml()
{

    bool finished_upload{false};
    /* First packet must be phyphox header*/
    if(header_sent == false)
    {
        header_sent = true;
        exp_notify_params.data = &exp_header[0];
        exp_notify_params.len = phyphox_autogen::HEADER_SIZE;
    }
    else
    {
        exp_xml_data_count+=MAX_PACKET_EXP_XML;
        uint8_t send_len{MAX_PACKET_EXP_XML};
        if(exp_xml_data_count > phyphox_autogen::EXP_DATA_SIZE)
        {
            const uint16_t extra = exp_xml_data_count- phyphox_autogen::EXP_DATA_SIZE;
            send_len -=extra;
            finished_upload = true;
        }
        exp_notify_params.data = &exp_data[exp_xml_next_idx];
        exp_notify_params.len = send_len;
        exp_xml_next_idx += send_len;
    }
    const int gatt_res = bt_gatt_notify_cb(nullptr,&exp_notify_params);
    __ASSERT(gatt_res==0,"Could not start notification of experiment");
    if(finished_upload)
    {
        if(exp_load::user_cb != nullptr)
        {
            exp_load::user_cb(exp_load::user_args);
        }
    }
}

} // namespace  phyphox_ble
