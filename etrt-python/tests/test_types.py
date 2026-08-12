#!/usr/bin/env python3
"""
Tests for etrt type bindings
"""

import pytest
import etrt


def test_device_properties():
    """Test DeviceProperties struct"""
    props = etrt.DeviceProperties()

    # Test fields are accessible
    props.frequency = 1000
    props.available_shires = 32
    props.memory_size = 1024 * 1024 * 1024

    assert props.frequency == 1000
    assert props.available_shires == 32
    assert props.memory_size == 1024 * 1024 * 1024


def test_form_factor_enum():
    """Test FormFactor enum"""
    assert etrt.FormFactor.PCIE
    assert etrt.FormFactor.M2


def test_arch_revision_enum():
    """Test ArchRevision enum"""
    assert etrt.ArchRevision.ETSOC1
    assert etrt.ArchRevision.PANTERO
    assert etrt.ArchRevision.GEPARDO
    assert etrt.ArchRevision.UNKNOWN


def test_error_context():
    """Test ErrorContext struct"""
    ctx = etrt.ErrorContext()
    ctx.type = 1
    ctx.cycle = 12345
    ctx.hart_id = 0
    ctx.mepc = 0x1000
    ctx.user_defined_error = -1

    assert ctx.type == 1
    assert ctx.cycle == 12345
    assert ctx.hart_id == 0
    assert ctx.mepc == 0x1000
    assert ctx.user_defined_error == -1


def test_load_code_result():
    """Test LoadCodeResult struct (mostly unused - load_code() returns tuple)"""
    result = etrt.LoadCodeResult()
    # LoadCodeResult exists for completeness but load_code() returns tuple
    # Fields should be accessible and work with integers
    result.event = 123
    result.kernel = 456
    result.load_address = 0x10000

    # Note: event and kernel are enum classes internally but exposed as ints
    # This is mainly for API completeness; most users will use the tuple from load_code()


def test_user_trace():
    """Test UserTrace struct"""
    trace = etrt.UserTrace()
    trace.buffer = 0x10000
    trace.buffer_size = 4096
    trace.threshold = 1024
    trace.shire_mask = 0xFF
    trace.thread_mask = 0xFFFFFFFF
    trace.event_mask = 0x1
    trace.filter_mask = 0x0

    assert trace.buffer == 0x10000
    assert trace.buffer_size == 4096
    assert trace.threshold == 1024


def test_dma_info():
    """Test DmaInfo struct"""
    info = etrt.DmaInfo()
    info.max_element_size = 32 * 1024 * 1024
    info.max_element_count = 4

    assert info.max_element_size == 32 * 1024 * 1024
    assert info.max_element_count == 4


def test_id_types():
    """Test that ID types are plain integers in Python"""
    # EventId, StreamId, DeviceId, KernelId are exposed as plain Python ints
    # They are enum classes in C++ but automatically converted to/from int
    # This design choice makes the API more Pythonic and avoids enum conversion issues

    # These types don't exist in Python - we use plain ints instead
    assert not hasattr(etrt, 'EventId')
    assert not hasattr(etrt, 'StreamId')
    assert not hasattr(etrt, 'DeviceId')
    assert not hasattr(etrt, 'KernelId')

    # The API functions accept and return plain ints for all ID types
    # Example: runtime.create_stream(device) returns int
    # Example: runtime.wait_for_event(event, timeout) accepts int


if __name__ == "__main__":
    pytest.main([__file__, "-v"])
