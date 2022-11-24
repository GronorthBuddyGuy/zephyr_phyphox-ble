/*!*****************************************************************
* Copyright 2022, Victor Chavez
* SPDX-License-Identifier: GPL-3.0-or-later
* @file phyphox_ble.hpp
* @author Victor Chavez (chavez-bermudez@fh-aachen.de)
* @date Nov 23, 2022
*
* @brief
* Phyphox ble zephyr interface
*
* @par Dependencies
* - language: C++17
* - OS: Zephyr
********************************************************************/
#pragma once
#include <cstdint>
#include <uuid_utils.hpp>

/*!< zephyr phyphox ble module */ 
namespace phyphox_ble
{
    /*!< UUIDs for phyphox */ 
    namespace uuid
    {
        /*!< Phyphox UUID base https://phyphox.org/wiki/index.php/Bluetooth_Low_Energy */
        static constexpr bt_uuid_128 base = BT_UUID_INIT_128(BT_UUID_128_ENCODE(0xCDDF0000, 
                                                                    0x30F7,
                                                                    0x4671,
                                                                    0x8B43,
                                                                    0x5E40BA53514A));

        /*!  Configuration Service UUID, typically attached to advertisement data to recognize
        *    the ble peripheral as phyphox compatible in the app.
        */
        static constexpr bt_uuid_128 SERVICE = ble::utils::uuid::derive_uuid(base,0x0001);
        /*!< phyphox characteristic UUIDs */
        namespace charact
        {
            static constexpr bt_uuid_128 EXP_XML= ble::utils::uuid::derive_uuid(base,0x0002);
            static constexpr bt_uuid_128 EVENT = ble::utils::uuid::derive_uuid(base,0x0004);
        }
    } // namespace uuid

     /*!< Main API for phyphox experiment */ 
    namespace experiment
    {
        /*! Types of Events that originate from phy phox app while running
            an experiment
        */
        enum class EventTypes : uint8_t
        {
            PAUSE = 0,  /*!< Experiment was paused*/
            START = 1, /*!< Experiment was started*/
            SYNC = 0xFF /*!< Phy Phox has connected*/
        };

        /*! Phy Phox Event data structure*/
        struct __attribute__((__packed__)) Event_t
        {
            EventTypes evt_type;    /* Type of event */
            uint64_t exp_time_ms;   /* Time the experiment has been running in ms*/
            uint64_t unix_time_ms;  /* Unix Time in ms from phyphox app*/
        };

        static constexpr uint8_t EVENT_SIZE{17U};
        static_assert(sizeof(Event_t)==EVENT_SIZE,"Phyphox event struct does not match EVENT_SIZE");


        /*! @brief Callback for events originated from phyphox app
            @param evt The event that was triggered
            @param args User arguments passed to the function
        */
        using event_cb = void (*)(const Event_t evt, void * args);


        /*! @brief dynamically set the title of the experiment
            @param [in] pData Buffer to new phyphox title of experiment
            @param [in] len Length of the buffer
            @returns true - if success<br>
                     false - if len is greater than allowed or pData is nullptr
        */
        bool set_title(char * pData,uint8_t len);
        
        /*! @brief dynamically set the ble in device name
            @details changes the ble name filter used by the phyphox experiment
            @note  It assumes there is only one unique ble service as input. 
                    The phyphox experiment should use as placeholder: "ble_name_in_placeholder"
            @param [in] pData Buffer to new ble name input
            @param [in] len Length of the buffer
            @returns true - if success<br>
                     false - if len is greater than allowed or pData is nullptr
        */
        bool set_blename_in(char * pData,uint8_t len);

        /*! @brief Update the crc of the experiment after using any of the
                    functions located in this namespace
        */
        void update_crc();

        /*! @brief Callback to report service has sent experiment to phyphox app
            @param [in] args User arguments passed to the function
        */
        using load_cb =  void (*)(void * args);

        /*! @brief Register a callback for phyphox experiment events
            @param [in] cb Callback for phyphox events
            @param [in] args User arguments passed to the function
        */
        void register_evt_cb(event_cb cb,void * args);

        /*! @brief Register a callback whenever the phyphox experiment has been loaded to phyphox app
            @param [in] cb Callback for whenever phphox has loaded experiment
            @param [in] args args User arguments passed to the function
        */
        void register_load_cb(load_cb cb,void * args);
    } // namespace experiment

} // namespace phyphox_ble
