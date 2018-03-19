#include "dxvk_buffer.h"
#include "dxvk_device.h"

namespace dxvk {
  
  DxvkBuffer::DxvkBuffer(
          DxvkDevice*           device,
    const DxvkBufferCreateInfo& createInfo,
          VkMemoryPropertyFlags memoryType)
  : m_device        (device),
    m_info          (createInfo),
    m_memFlags      (memoryType) {
    // Align physical buffer slices to 256 bytes, which guarantees
    // that we don't violate any Vulkan alignment requirements
    m_physSliceLength = createInfo.size;
    m_physSliceStride = align(createInfo.size, 256);
    
    // Allocate a single buffer slice
    m_physSlice = this->allocPhysicalBuffer(1)
      ->slice(0, m_physSliceStride);
  }
  
  
  DxvkPhysicalBufferSlice DxvkBuffer::rename(const DxvkPhysicalBufferSlice& slice) {
    DxvkPhysicalBufferSlice prevSlice = std::move(m_physSlice);
    
    m_physSlice = slice;
    m_revision += 1;
    return prevSlice;
  }
  
  
  DxvkPhysicalBufferSlice DxvkBuffer::allocPhysicalSlice() {
    std::unique_lock<std::mutex> lock(m_mutex);
    
    // If necessary, create a new buffer
    // that we'll allocate slices from.
    if (m_slices.size() == 0) {
      const Rc<DxvkPhysicalBuffer> buffer
        = this->allocPhysicalBuffer(m_physSliceCount);
      
      for (uint32_t i = 0; i < m_physSliceCount; i++) {
        m_slices.push_back(buffer->slice(
          m_physSliceStride * i,
          m_physSliceLength));
      }
      
      m_physSliceCount *= 2;
    }
    
    // Take the first slice from the queue
    DxvkPhysicalBufferSlice result = std::move(m_slices.back());
    m_slices.pop_back();
    return result;
  }
  
  
  void DxvkBuffer::freePhysicalSlice(const DxvkPhysicalBufferSlice& slice) {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_slices.push_back(slice);
  }
  
  
  Rc<DxvkPhysicalBuffer> DxvkBuffer::allocPhysicalBuffer(VkDeviceSize sliceCount) const {
    DxvkBufferCreateInfo createInfo = m_info;
    createInfo.size = sliceCount * m_physSliceStride;
    
    return m_device->allocPhysicalBuffer(createInfo, m_memFlags);
  }
  
  
  DxvkBufferView::DxvkBufferView(
    const Rc<vk::DeviceFn>&         vkd,
    const Rc<DxvkBuffer>&           buffer,
    const DxvkBufferViewCreateInfo& info)
  : m_vkd(vkd), m_info(info), m_buffer(buffer),
    m_physView(this->createView()),
    m_revision(m_buffer->m_revision) {
    
  }
  
  
  DxvkBufferView::~DxvkBufferView() {
    
  }
  
  
  void DxvkBufferView::updateView() {
    if (m_revision != m_buffer->m_revision) {
      m_physView = this->createView();
      m_revision = m_buffer->m_revision;
    }
  }
  
  
  Rc<DxvkPhysicalBufferView> DxvkBufferView::createView() {
    return new DxvkPhysicalBufferView(
      m_vkd, m_buffer->slice(), m_info);
  }
  
  
  DxvkBufferTracker:: DxvkBufferTracker() { }
  DxvkBufferTracker::~DxvkBufferTracker() { }
  
  
  void DxvkBufferTracker::freeBufferSlice(
    const Rc<DxvkBuffer>&           buffer,
    const DxvkPhysicalBufferSlice&  slice) {
    m_entries.push_back({ buffer, slice });
  }
  
  
  void DxvkBufferTracker::reset() {
    for (const auto& e : m_entries)
      e.buffer->freePhysicalSlice(e.slice);
      
    m_entries.clear();
  }
  
}